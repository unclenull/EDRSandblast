#pragma once
#include <Windows.h>

DWORD64 FindNtoskrnlBaseAddress(void);
TCHAR* FindDriverName(DWORD64 address, _Out_opt_ PDWORD64 offset);
TCHAR* FindDriverPath(DWORD64 address);
DWORD64 GetKernelFunctionAddress(LPCSTR function);

TCHAR* GetDriverName(DWORD64 address);
DWORD64 FindDriverBaseAddress(const TCHAR* driverName);

#define STRING_MAX_LENGTH 256
WORD ReadUnicodeString(DWORD64 Address, wchar_t buffer[STRING_MAX_LENGTH]);
