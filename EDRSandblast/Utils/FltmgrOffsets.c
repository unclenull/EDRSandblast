/*

--- fltmgr Notify Routines' offsets from CSV functions.
--- Hardcoded patterns, with offsets for 350+ fltmgr versions provided in the CSV file.

*/
#include <tchar.h>
#include <stdio.h>

#include "FileVersion.h"
#include "../EDRSandblast.h"

#include "FltmgrOffsets.h"

DWORD64 g_fltmgrOffsets[_FLTOS_COUNT] = { 0 };

BOOL FltmgrOffsetsArePresent() {
  return g_fltmgrOffsets[0] > 0;
}

// Return the offsets of nt!PspCreateProcessNotifyRoutine, nt!PspCreateThreadNotifyRoutine, nt!PspLoadImageNotifyRoutine, and nt!_PS_PROTECTION for the specific Windows version in use.
void LoadFltmgrOffsetsFromFile(TCHAR* fltmgrOffsetFilename) {
    LPTSTR fltmgrVersion = GetFltmgrVersion();
    _tprintf_or_not(TEXT("[*] System's fltmgr.sys file version is: %s\n"), fltmgrVersion);

    FILE* offsetFileStream = NULL;
    _tfopen_s(&offsetFileStream, fltmgrOffsetFilename, TEXT("r"));

    if (offsetFileStream == NULL) {
        _putts_or_not(TEXT("[!] Offset CSV file connot be opened"));
        return;
    }

    TCHAR lineFltmgrVersion[2048];
    TCHAR line[2048];
    while (_fgetts(line, _countof(line), offsetFileStream)) {
        if (_tcsncmp(line, TEXT("fltmgr"), _countof(TEXT("fltmgr")) - 1)) {
            _putts_or_not(TEXT("[-] CSV file format is unexpected!\n"));
            break;
        }
        TCHAR* dupline = _tcsdup(line);
        TCHAR* tmpBuffer = NULL;
        _tcscpy_s(lineFltmgrVersion, _countof(lineFltmgrVersion), _tcstok_s(dupline, TEXT(","), &tmpBuffer));
        if (_tcscmp(fltmgrVersion, lineFltmgrVersion) == 0) {
            TCHAR* endptr;
            _tprintf_or_not(TEXT("[+] Offsets are available for this version of fltmgr.sys (%s)!\n"), fltmgrVersion);
            for (int i = 0; i < _FLTOS_COUNT; i++) {
                g_fltmgrOffsets[i] = _tcstoull(_tcstok_s(NULL, TEXT(","), &tmpBuffer), &endptr, 16);
            }
            break;
        }
    }
    fclose(offsetFileStream);
}

void PrintFltmgrOffsets() {
    _tprintf_or_not(TEXT("[+] Fltmgr offsets: "));
    for (int i = 0; i < _FLTOS_COUNT - 1; i++) {
        _tprintf_or_not(TEXT(" %llx |"), g_fltmgrOffsets[i]);
    }
    _tprintf_or_not(TEXT("%llx\n"), g_fltmgrOffsets[_FLTOS_COUNT - 1]);
}

TCHAR g_fltmgrPath[MAX_PATH] = { 0 };
LPTSTR GetFltmgrPath() {
    if (_tcslen(g_fltmgrPath) == 0) {
        // Retrieves the system folder (eg C:\Windows\System32).
        TCHAR systemDirectory[MAX_PATH] = { 0 };
        GetSystemDirectory(systemDirectory, _countof(systemDirectory));

        // Compute fltmgr.exe path.
        _tcscat_s(g_fltmgrPath, _countof(g_fltmgrPath), systemDirectory);
        _tcscat_s(g_fltmgrPath, _countof(g_fltmgrPath), TEXT("\\drivers"));
        _tcscat_s(g_fltmgrPath, _countof(g_fltmgrPath), TEXT("\\fltmgr.sys"));
    }
    return g_fltmgrPath;
}

TCHAR g_fltmgrVersion[256] = { 0 };
LPTSTR GetFltmgrVersion() {
  if (_tcslen(g_fltmgrVersion) == 0) {
    LPTSTR fltmgrPath = GetFltmgrPath();
    TCHAR versionBuffer[256] = { 0 };
    GetFileVersion(versionBuffer, _countof(versionBuffer), fltmgrPath);
    _stprintf_s(g_fltmgrVersion, 256, TEXT("fltmgr_%s.exe"), versionBuffer);
  }
  return g_fltmgrVersion;
}
