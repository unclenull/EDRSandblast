#include <Windows.h>
#include <winternl.h>
#include <Tchar.h>
#include <stdio.h>
#include "KernelMemoryPrimitives.h"
#include "KernelUtils.h"
#include "./symbol.h"
#include "./memory.h"
#include "./kernel.h"
#include "./netio.h"

#pragma comment(lib, "ntdll.lib")

OS_KEY BuildIDKey = 0;

BUILDID_KEY_MAP BuildidKeyMap[] = {
  {2600, OS_XP},
  {3790, OS_2K3},	
  {6000, OS_VISTA},
  {6001, OS_VISTA},
  {6002, OS_VISTA},
  {7600, OS_7},
  {7601, OS_7},
  {8102, OS_8},
  {8250, OS_8},
  {9200, OS_8},
  {9600, OS_BLUE},
  {10240, OS_10_10240},
  {10586, OS_10_10586},
  {14393, OS_10_14393},
  {15063, OS_10_15063},
  {16299, OS_10_16299},
  {17134, OS_10_17134},
  {17763, OS_10_17763},
  {18362, OS_10_18362},
  {18363, OS_10_18363},
  {19041, OS_10_19041},
  {19042, OS_10_19042},
  {19043, OS_10_19043},
  {19044, OS_10_19044},
  {19045, OS_10_19045},
  {20348, OS_10_20348},
  {22000, OS_10_22000},
  {22621, OS_10_22621},
  {22631, OS_10_22631}
};

MODULE ModuleWhiteList[] = {
  FOREACH_MODULE(MODULE_ITEM)
};

#ifdef _DEBUG
PMODULE FullModuleList;
ULONG FullModuleListCount;
void fillFullModuleList(PSYSTEM_MODULE_INFORMATION pModuleList) {
  FullModuleListCount = pModuleList->Count;
  FullModuleList = (PMODULE) LocalAlloc(LMEM_ZEROINIT, sizeof(MODULE) * FullModuleListCount);
  for (ULONG i = 0; i < FullModuleListCount; i++)
  {
    PMODULE pM = &FullModuleList[i];
    pM->name = pModuleList->Module[i].ImageName + pModuleList->Module[i].PathLength;
    pM->start = pModuleList->Module[i].Base;
    pM->end = pModuleList->Module[i].Base + pModuleList->Module[i].Size;
  }
}

void BelongToDriver(DWORD64 addr) {
  for (ULONG i = 0; i < FullModuleListCount; i++) {
    PMODULE pM = &FullModuleList[i];
    if (pM->start < addr && pM->end > addr) {
      printf("Belongs to driver [%s]: %llx\n", pM->name, addr);
      return;
    }
  }
  printf("Failed to find the driver for address: %llx\n", addr);
}
#endif


WORD readVersion() {
#if defined(_WIN64)
#define PEBOffset 0x60
#define OSBuildNumberOffset 0x120
  PBYTE pPeb = (PBYTE)__readgsqword(PEBOffset);
#else
#define PEBOffset 0x30
#define OSBuildNumberOffset 0xac
  PBYTE *pPeb = (PBYTE)__readfsdword(PEBOffset);
#endif

  WORD version = *(WORD *)(pPeb + OSBuildNumberOffset);
  return version;
}

BOOL IsAddressWhitelisted(MODULE_ID *list, UINT8 size, DWORD64 addr) {
  for (UINT8 j = 0; j < size; j++) {
    PMODULE pM = &ModuleWhiteList[list[j]];
    if (pM->start < addr && pM->end > addr) {
      printf("In Whitelist [%s]: %llx\n", pM->name, addr);
      return TRUE;
    }
  }
  return FALSE;
}

void fillExports(PMODULE_PATCHED pM) {
  DWORD64 baseAddress = pM->start;
  UINT8 funcSize = pM->exportsCount;
  // IMAGE_NT_HEADERS32 in x86
  DWORD64 ntHeaders = baseAddress + ReadMemoryDWORD(baseAddress + offsetof(IMAGE_DOS_HEADER, e_lfanew));
  DWORD64 exportDirectory = ntHeaders + offsetof(IMAGE_NT_HEADERS, OptionalHeader) + offsetof(IMAGE_OPTIONAL_HEADER, DataDirectory) + sizeof(IMAGE_DATA_DIRECTORY) * IMAGE_DIRECTORY_ENTRY_EXPORT;
  DWORD exportDirectoryVirtualAddress = ReadMemoryDWORD(exportDirectory + offsetof(IMAGE_DATA_DIRECTORY, VirtualAddress));
  DWORD64 FileHeader = ntHeaders + offsetof(IMAGE_NT_HEADERS, FileHeader);
  WORD SizeOfOptionalHeader = ReadMemoryWORD(FileHeader + offsetof(IMAGE_FILE_HEADER, SizeOfOptionalHeader));
  WORD NumberOfSections = ReadMemoryWORD(FileHeader + offsetof(IMAGE_FILE_HEADER, NumberOfSections));
  DWORD64 sectionHeader = (DWORD64)((ULONG_PTR)(ntHeaders) + ((LONG)(LONG_PTR)&(((IMAGE_NT_HEADERS *)0)->OptionalHeader)) + SizeOfOptionalHeader);
  for (DWORD i = 0; i < NumberOfSections; i++, sectionHeader += sizeof(IMAGE_SECTION_HEADER)) {
    DWORD sectionVirtualAddress = ReadMemoryDWORD(sectionHeader + offsetof(IMAGE_SECTION_HEADER, VirtualAddress));
    DWORD sectionSize = ReadMemoryDWORD(sectionHeader + offsetof(IMAGE_SECTION_HEADER, SizeOfRawData));
    if ((exportDirectoryVirtualAddress >= sectionVirtualAddress) &&
        (exportDirectoryVirtualAddress < sectionVirtualAddress + sectionSize)) {
      break;
    }
  }

  DWORD64 exportTable = baseAddress + exportDirectoryVirtualAddress;
  DWORD64 addressOfFunctions = baseAddress + ReadMemoryDWORD(exportTable + offsetof(IMAGE_EXPORT_DIRECTORY, AddressOfFunctions));
  DWORD64 addressOfNames = baseAddress + ReadMemoryDWORD(exportTable + offsetof(IMAGE_EXPORT_DIRECTORY, AddressOfNames));
  DWORD64 addressOfNameOrdinals = baseAddress + ReadMemoryDWORD(exportTable + offsetof(IMAGE_EXPORT_DIRECTORY, AddressOfNameOrdinals));
  DWORD numberOfNames = ReadMemoryDWORD(exportTable + offsetof(IMAGE_EXPORT_DIRECTORY, NumberOfNames));
  UINT8 *foundIx = (UINT8 *)LocalAlloc(LMEM_ZEROINIT, sizeof(UINT8) * funcSize);
  UINT8 foundTotal = 0;
  PCHAR pName = (PCHAR)LocalAlloc(LMEM_ZEROINIT, STRING_MAX_LENGTH); 
  for (DWORD i = 0; i < numberOfNames; i++) {
    DWORD64 nameKnl = baseAddress + ReadMemoryDWORD(addressOfNames + sizeof(DWORD) * i);
    ReadAnsiString(nameKnl, pName);
    for (UINT8 j = 0; j < funcSize; j++) {
      if (foundIx[j]) continue;
      UINT8 id = pM->exportIds[j];
      PCHAR func = pM->exportNames[id];
      if (strcmp(pName, func) == 0) {
        DWORD ix = ReadMemoryWORD(addressOfNameOrdinals + sizeof(WORD) * i); // 2B
        DWORD64 addr = baseAddress + ReadMemoryDWORD(addressOfFunctions + sizeof(DWORD) * ix);
        pM->exportAddresses[id] = addr;
        printf("Found function [%s]: %llx\n", pName, addr);
        foundIx[j] = TRUE;
        foundTotal += 1;
        if (foundTotal == funcSize) {
          return;
        }
        break;
      }
    }
  }
}

void initWindowsId(WORD buildId) {
  int size = _countof(BuildidKeyMap);
  for (int i = 0; i < size; i++) {
    BUILDID_KEY_MAP map = BuildidKeyMap[i];
    if (map.buildId == buildId) {
      BuildIDKey = map.key;
      return;
    }
  }
  BuildIDKey = BuildidKeyMap[size - 1].key;
}

PSYSTEM_MODULE_INFORMATION getFullDriverList() {
  ULONG length = 0;

  NtQuerySystemInformation(11, NULL, 0, &length);
  PSYSTEM_MODULE_INFORMATION pModuleList = (PSYSTEM_MODULE_INFORMATION)LocalAlloc(LMEM_ZEROINIT, length);
  NtQuerySystemInformation(11, pModuleList, length, &length);

  return pModuleList;
}

void fillModuleAddress(PSYSTEM_MODULE_INFORMATION pModuleList, PMODULE pM) {
  for (ULONG i = 0; i < pModuleList->Count; i++)
  {
    if (_stricmp(pM->name, pModuleList->Module[i].ImageName + pModuleList->Module[i].PathLength) == 0) {
      pM->start = pModuleList->Module[i].Base;
      pM->end = pModuleList->Module[i].Base + pModuleList->Module[i].Size;
      printf("Found %s at %llX size %i\n", pM->name, pM->start, pModuleList->Module[i].Size);
      return;
    }
  }
  printf("Failed to find %s\n", pM->name);
}

void resolveSymbols(PMODULE_PATCHED pM) {
  printf("Resolve symbols for %s\n", pM->name);
  for (int j = 0; j < pM->symbolsCount; j++) {
    PSYMBOL_META_POOL_ITEM pItem = &pM->symbolMetaPool[j];
    PSYMBOL_META pMeta = pItem->metaGetter();
    DWORD64 symbolOffsetAddr;
    if (pMeta->pattern) {
      DWORD64 patternAddr = patternSearch(pM->exportAddresses[pMeta->exportId], pMeta->range, pMeta->pattern);
      symbolOffsetAddr = patternAddr + pMeta->offset;
    } else {
      symbolOffsetAddr = pM->exportAddresses[pMeta->exportId] + pMeta->offset;
    }
    DWORD symbolOffset = ReadMemoryDWORD(symbolOffsetAddr);
    pM->symbolsAddresses[pItem->id] = symbolOffsetAddr + sizeof(DWORD) + symbolOffset;
    printf("symbol No.%2i: %llx\n", j, pM->symbolsAddresses[pItem->id]);
  }
}

PBYTE FindVersionedStruct(PBYTE start, UINT8 itemCount, UINT8 itemSize) {
  PBYTE nearYounger = 0;
  for (int i = 0; i < itemCount; i++, start += itemSize) {
    OS_KEY key = *(OS_KEY *)start;
    if (key == BuildIDKey) {
      return start;
    } else if (key < BuildIDKey) {
      nearYounger = start;
    }
  }
  return nearYounger;
}

void InitModulesAndSymbols() {
    DebugBreak();

  // Get build id
  initWindowsId(readVersion());

  // Init modules
  PSYSTEM_MODULE_INFORMATION moduleRawList = getFullDriverList();

#ifdef _DEBUG
  fillFullModuleList(moduleRawList);
#endif

  // Modules whitelist
  for (int i = 0; i < _countof(ModuleWhiteList); i++) {
    PMODULE pM = &ModuleWhiteList[i];
    fillModuleAddress(moduleRawList, pM);
  }

  // Modules to patch
  PMODULE_PATCHED modulesPatched[] = { &ModuleKernel, &ModuleNetio };
  for (int i = 0; i < _countof(modulesPatched); i++) {
    PMODULE_PATCHED pM = modulesPatched[i];

    // Initiate exports list if dynamic
    if (pM->exportsInitializer) {
      pM->exportsInitializer(pM);
    }

    // Resolve export addresses
    fillModuleAddress(moduleRawList, (PMODULE)pM);
    fillExports(pM);

    // Calculate symbols addresses.
    resolveSymbols(pM);
  }

  LocalFree(moduleRawList);
}

