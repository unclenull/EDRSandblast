#include "../EDRSandblast.h"
#include "IsEDRChecks.h"

/*
* Primitives to check if a binary or driver belongs to an EDR product.
*/

// List of keywords matching EDR companies as employed for binary digitial signatures.
// TODO : enrich this list
TCHAR const* EDR_SIGNATURE_KEYWORDS[] = {
   _T("CarbonBlack"),
   _T("CrowdStrike"),
   _T("Cylance Smart Antivirus"),
   _T("Elastic Endpoint Security"),
   _T("FireEye"),
   _T("Kaspersky"),
   _T("McAfee"),
   _T("SentinelOne"),
   _T("Sentinel Labs"),
   _T("Symantec")
};

// List of binaries belonging to EDR products.
TCHAR const* EDR_BINARIES[] = {
    // Microsoft
   _T("HealthService.exe"),
   _T("MonitoringHost.exe"),
   _T("MpCmdRun.exe"),
   _T("MsMpEng.exe"),
   _T("MsSense.exe"),
   _T("SenseCncProxy.exe"),
   _T("SenseIR.exe"),
   // SentinelOne
   _T("LogCollector.exe"),
   _T("SentinelAgent.exe"),
   _T("SentinelAgentWorker.exe"),
   _T("SentinelBrowserNativeHost.exe"),
   _T("SentinelHelperService.exe"),
   _T("SentinelMemoryScanner.exe"),
   _T("SentinelRanger.exe"),
   _T("SentinelRemediation.exe"),
   _T("SentinelRemoteShellHost.exe"),
   _T("SentinelScanFromContextMenu.exe"),
   _T("SentinelServiceHost"),
   _T("SentinelStaticEngine.exe"),
   _T("SentinelStaticEngineScanner.exe"),
   _T("SentinelUI.exe"),
};

// List of EDR drivers for which Kernel callbacks will be impacted.
// Source: https://docs.microsoft.com/en-us/windows-hardware/drivers/ifs/allocated-altitudes
// Includes all FSFilter Anti-Virus and Activity Monitor drivers.
// and : https://github.com/SadProcessor/SomeStuff/blob/master/Invoke-EDRCheck.ps1
TCHAR const* EDR_DRIVERS[] = {
_T("ntoskrnl.exe"),
_T("ntoskrnl.exe"),
_T("wcnfs.sys"),
_T("bindflt.sys"),
_T("cldflt.sys"),
_T("iorate.sys"),
_T("ioqos.sys"),
_T("fsdepends.sys"),
_T("sftredir.sys"),
_T("dfs.sys"),
_T("csvnsflt.sys"),
_T("csvflt.sys"),
_T("Microsoft.Uev.AgentDriver.sys"),
_T("AppvVfs.sys"),
_T("CCFFilter.sys"),
_T("defragger.sys"),
_T("DhWatchdog.sys"),
_T("mssecflt.sys"),
_T("Backupreader.sys"),
_T("MsixPackagingToolMonitor.sys"),
_T("AppVMon.sys"),
_T("DpmFilter.sys"),
_T("Procmon11.sys"),
_T("UCPD.sys"),
_T("wtd.sys"),
_T("minispy.sys - Top"),
_T("fdrtrace.sys"),
_T("filetrace.sys"),
_T("uwfreg.sys"),
_T("uwfs.sys"),
_T("locksmith.sys"),
_T("winload.sys"),
_T("CbSampleDrv.sys"),
_T("CbSampleDrv.sys"),
_T("CbSampleDrv.sys"),
_T("simrep.sys"),
_T("change.sys"),
_T("delete_flt.sys"),
_T("SmbResilFilter.sys"),
_T("usbtest.sys"),
_T("NameChanger.sys"),
_T("failMount.sys"),
_T("failAttach.sys"),
_T("stest.sys"),
_T("cdo.sys"),
_T("ctx.sys"),
_T("fmm.sys"),
_T("cancelSafe.sys"),
_T("message.sys"),
_T("passThrough.sys"),
_T("nullFilter.sys"),
_T("ntest.sys"),
_T("minispy.sys - Middle"),
_T("iiscache.sys"),
_T("wrpfv.sys"),
_T("msnfsflt.sys"),
_T("minispy.sys - Bottom"),
_T("WinSetupMon.sys"),
_T("WdFilter.sys"),
_T("mpFilter.sys"),
_T("dvfilter.sys"),
_T("fsrecord.sys"),
_T("defilter.sys (old)"),
_T("avscan.sys"),
_T("scanner.sys"),
_T("CCFFilter.sys"),
_T("cbafilt.sys"),
_T("SmbBandwidthLimitFilter.sys"),
_T("DfsrRo.sys"),
_T("DataScrn.sys"),
_T("storqosflt.sys"),
_T("fbwf.sys"),
_T("fsredir.sys"),
_T("ResumeKeyFilter.sys"),
_T("wcifs.sys"),
_T("prjflt.sys"),
_T("p4vfsflt.sys"),
_T("gameflt.sys"),
_T("odphflt.sys"),
_T("cldflt.sys"),
_T("dvfilter.sys"),
_T("Accesstracker.sys"),
_T("Changetracker.sys"),
_T("Fstier.sys"),
_T("appxstrm.sys"),
_T("wimmount.sys"),
_T("hsmflt.sys"),
_T("dfsrflt.sys"),
_T("StorageSyncGuard.sys"),
_T("StorageSync.sys"),
_T("dedup.sys"),
_T("dfmflt.sys"),
_T("sis.sys"),
_T("wimFltr.sys"),
_T("compress.sys"),
_T("cmpflt.sys"),
_T("wimfsf.sys"),
_T("prvflder.sys"),
_T("Filecrypt.sys"),
_T("encrypt.sys"),
_T("swapBuffers.sys"),
_T("svhdxflt.sys"),
_T("luafv.sys"),
_T("MEMEPMAgent.sys"),
_T("sfo.sys"),
_T("DeVolume.sys"),
_T("quota.sys"),
_T("scrubber.sys"),
_T("Npsvctrig.sys"),
_T("rsfxdrv.sys"),
_T("defilter.sys"),
_T("AppVVemgr.sys"),
_T("wofadk.sys"),
_T("wof.sys"),
_T("fileinfo"),
_T("WinSetupBoot.sys"),
_T("WinSetupMon.sys")
};

BOOL isFileSignatureMatchingEDR(TCHAR* filePath) {
    SignatureOpsError returnValue;
    TCHAR* signers = NULL;
    size_t szSigners = 0;
    returnValue = GetFileSigners(filePath, signers, &szSigners);

    // Expected if the file is signed, first call will return the needed buffer size.
    if (returnValue == E_INSUFFICIENT_BUFFER) {
        signers = calloc(szSigners, sizeof(TCHAR));
        if (!signers) {
            _tprintf_or_not(TEXT("[!] Couldn't allocate memory for Signers information for binary \"%s\"\n"), filePath);
            return FALSE;
        }
        returnValue = GetFileSigners(filePath, signers, &szSigners);
    }

    // If the file is not signed, it's unlikely to be linked to an EDR product.
    if (returnValue == E_NOT_SIGNED) {
        // _tprintf_or_not(TEXT("[*] File \"%s\" is not signed.\n"), binaryPath);
        return FALSE;
    }

    if (returnValue == E_FILE_NOT_FOUND) {
        _tprintf_or_not(TEXT("[!] Couldn't locate file \"%s\" to retrieve certificate information.\n"), filePath);
        return FALSE;
    }

    if ((returnValue != E_SUCCESS) || !signers) {
        _tprintf_or_not(TEXT("[!] An error occurred while retrieving certificate information for file \"%s\"\n"), filePath);
        return FALSE;
    }

    // Iterates over each keywords in EDR_SIGNATURE_KEYWORDS and return TRUE if a match is found.
    for (int i = 0; i < _countof(EDR_SIGNATURE_KEYWORDS); ++i) {
        if (_tcsstr(signers, EDR_SIGNATURE_KEYWORDS[i])) {
            free(signers);
            return TRUE;
        }
    }

    free(signers);
    return FALSE;
}

BOOL isBinaryNameMatchingEDR(TCHAR* binaryName) {
    for (int i = 0; i < _countof(EDR_BINARIES); ++i) {
        if (_tcsicmp(binaryName, EDR_BINARIES[i]) == 0) {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL isBinaryPathMatchingEDR(TCHAR* binaryPath) {
    for (int i = 0; i < _countof(EDR_BINARIES); ++i) {
        if (_tcsstr(binaryPath, EDR_BINARIES[i])) {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL isDriverNameMatchingEDR(TCHAR* driverName) {
    for (int i = 0; i < _countof(EDR_DRIVERS); ++i) {
        if (_tcsicmp(driverName, EDR_DRIVERS[i]) == 0) {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL isDriverPathMatchingEDR(TCHAR* driverPath) {
    for (int i = 0; i < _countof(EDR_DRIVERS); ++i) {
        if (_tcsstr(driverPath, EDR_DRIVERS[i])) {
            return TRUE;
        }
    }
    return FALSE;
}

// TODO : create an API to check, with only the name of a loaded driver, if it an EDR (check its name against the hardcoded list of names, automatically find its path on disk and check the file signature)
