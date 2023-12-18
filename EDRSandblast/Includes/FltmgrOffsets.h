/*

--- fltmgr Notify Routines' offsets from CSV functions.
--- Hardcoded patterns, with offsets for 350+ fltmgr versions provided in the CSV file.

*/

#pragma once

#include <Windows.h>


enum FltmgrOffsetType {
    FLTOS_FltGlobals,
    FLTOS_GLOBALS__FrameList,
    FLTOS_RESOURCE_LIST_HEAD__rList,
    FLTOS_RESOURCE_LIST_HEAD__rCount,
    FLTOS_OBJECT_PrimaryLink,
    FLTOS_FLTP_FRAME__Links,
    FLTOS_FLTP_FRAME__RegisteredFilters,
    FLTOS_FLT_FILTER__Name,
    FLTOS_FLT_FILTER__InstanceList,
    FLTOS_FLT_FILTER__Registration,
    FLTOS_FLT_VOLUME__InstanceList,
    FLTOS_FLT_INSTANCE__Volume,
    FLTOS_FLT_INSTANCE__FilterLink,
    FLTOS_FLT_INSTANCE__CallbackNodes,
    _FLTOS_COUNT,
};

extern DWORD64* g_fltmgrOffsets;

BOOL FltmgrOffsetsArePresent();
// Stores, in a global variable, the offsets of nt!PspCreateProcessNotifyRoutine, nt!PspCreateThreadNotifyRoutine, nt!PspLoadImageNotifyRoutine, and nt!_PS_PROTECTION for the specific Windows version in use.
void LoadFltmgrOffsetsFromFile(TCHAR* fltmgrOffsetFilename);

// Print the Ntosknrl offsets.
void PrintFltmgrOffsets();
LPTSTR GetFltmgrVersion();
