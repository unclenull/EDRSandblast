#include "./symbol.h"
#include "./kernel.h"
PCHAR exportNamesKnl[] = {
   FOREACH_EXPORT_KNL(STR)
};

UINT8 kernelExports_os10_10586[] = {
  EXP_KNL_PsSetLoadImageNotifyRoutine,
  EXP_KNL_PsSetCreateProcessNotifyRoutine,
  EXP_KNL_PsRemoveCreateThreadNotifyRoutine,
  EXP_KNL_ObCreateObjectType,
  EXP_KNL_CmUnRegisterCallback,
  EXP_KNL_NtTraceEvent
};
UINT8 kernelExports_os10_ll[] = {
  EXP_KNL_PsSetLoadImageNotifyRoutine,
  EXP_KNL_PsSetCreateProcessNotifyRoutine,
  EXP_KNL_PsRemoveCreateThreadNotifyRoutine,
  EXP_KNL_ObCreateObjectType,
  EXP_KNL_CmUnRegisterCallback,
  EXP_KNL_EtwSendTraceBuffer
};
UINT8 kernelExports_os10_lg[] = {
  EXP_KNL_PsSetLoadImageNotifyRoutineEx,
  EXP_KNL_PsSetCreateProcessNotifyRoutine,
  EXP_KNL_PsRemoveCreateThreadNotifyRoutine,
  EXP_KNL_ObCreateObjectType,
  EXP_KNL_CmUnRegisterCallback,
  EXP_KNL_EtwSendTraceBuffer
};
// The same size of all IDs, since we use it for indexing
DWORD64 knlExportAddresses[EXP_KNL_COUNT] = { 0 };

BOOL initKernelExports(PMODULE_PATCHED pM) {
  if (BuildIDKey > OS_10) {
    if (BuildIDKey == OS_10_10586) {
      pM->exportIds = kernelExports_os10_10586;
      pM->exportsCount = _countof(kernelExports_os10_10586);
    } else if (BuildIDKey <= OS_10_17134) {
      pM->exportIds = kernelExports_os10_ll;
      pM->exportsCount = _countof(kernelExports_os10_ll);
    } else {
      pM->exportIds = kernelExports_os10_lg;
      pM->exportsCount = _countof(kernelExports_os10_lg);
    }
    return TRUE;
  } else {
    return FALSE;
  }
}

SYMBOL_META OS_10_PspLoadImageNotifyRoutine_LL = { EXP_KNL_PsSetLoadImageNotifyRoutine, 0x80, 0x48d90c8d48c03345, 0, -4, 0, 0 };
SYMBOL_META OS_10_PspLoadImageNotifyRoutine_LG = { EXP_KNL_PsSetLoadImageNotifyRoutineEx, 0x80, 0x48d90c8d48c03345, 0, -4, 0, 0 };
PSYMBOL_META getter_PspLoadImageNotifyRoutine() {
  if (BuildIDKey > OS_10) {
    if (BuildIDKey <= OS_10_17134) {
      return &OS_10_PspLoadImageNotifyRoutine_LL;
    } else {
      return &OS_10_PspLoadImageNotifyRoutine_LG;
    }
  }

  return NULL;
}

SYMBOL_META OS_10_WmipLoggerContext = { EXP_KNL_EtwSendTraceBuffer, 0xff, 0xF8BC8B4900000000, 0xFFFFFFFF00000000, 8, 1, 0 };
PSYMBOL_META getter_WmipLoggerContext() {
  if (BuildIDKey > OS_10) {
    if (BuildIDKey == OS_10_10240) { // Only this version stores it in a global symbol
      return &OS_10_WmipLoggerContext;
    } else {
      return NULL;
    }
  }

  return NULL;
}

SYMBOL_META OS_10_EtwpGetSiloDriverState = { EXP_KNL_NtTraceEvent, 0, 0, 0, 0x4DE, 0, 0 };
PSYMBOL_META getter_EtwpGetSiloDriverState() {
  if (BuildIDKey > OS_10) {
    if (BuildIDKey == OS_10_10586) { // Only this version stores in the field WmipLoggerContext of this global state
      return &OS_10_EtwpGetSiloDriverState;
    } else {
      return NULL;
    }
  }

  return NULL;
}

SYMBOL_META OS_10_EtwpSiloState = { SYM_KNL_EtwpGetSiloDriverState, 0, 0, 0, 0x2B, 0, 1 };
PSYMBOL_META getter_EtwpSiloState() {
  if (BuildIDKey > OS_10) {
    if (BuildIDKey == OS_10_10586) { // Only this version stores in the field WmipLoggerContext of this global state
      return &OS_10_EtwpSiloState;
    } else {
      return NULL;
    }
  }

  return NULL;
}

SYMBOL_META OS_10_EtwpHostSiloState = { EXP_KNL_EtwSendTraceBuffer, 0x80, 0x158B480000000000, 0xFFFFFF0000000000, 8, 0, 0 };
PSYMBOL_META getter_EtwpHostSiloState() {
  // EtwpHostSiloState is renamed from EtwpSiloState, the field WmipLoggerContext is renamed to EtwpLoggerContext from 16299
  if (BuildIDKey > OS_10) {
    if (BuildIDKey > OS_10_10586) {
      return &OS_10_EtwpHostSiloState;
    } else {
      return NULL;
    }
  }

  return NULL;
}

SYMBOL_META_POOL_ITEM symbolMetaPoolKnl[] = {
  {
    SYM_KNL_PspLoadImageNotifyRoutine, getter_PspLoadImageNotifyRoutine
  },
  {
    SYM_KNL_WmipLoggerContext, getter_WmipLoggerContext
  },
  {
    SYM_KNL_EtwpGetSiloDriverState, getter_EtwpGetSiloDriverState
  },
  {
    SYM_KNL_EtwpSiloState, getter_EtwpSiloState
  },
  {
    SYM_KNL_EtwpHostSiloState, getter_EtwpHostSiloState
  }
};

DWORD64 KernelSymbolAddresses[SYM_KNL_COUNT] = { 0 };

BOOL KernelSetup(PSYSTEM_MODULE_INFORMATION moduleRawList) {
  MODULE_PATCHED moduleKernel =
  { "ntoskrnl.exe", 0, 0, initKernelExports, 0, 0, exportNamesKnl, knlExportAddresses, SYM_KNL_COUNT, symbolMetaPoolKnl, KernelSymbolAddresses };
  return InitPatchedModule(moduleRawList, &moduleKernel);
}
