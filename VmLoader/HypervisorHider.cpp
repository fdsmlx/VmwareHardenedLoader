#include <ntddk.h>
#include <intrin.h>
#include "HypervisorHider.h"
#include "Config.h"
#include "Utils.h"

// Hypervisor Hider Module - Removes hypervisor presence indicators
// Cleans hypervisor-related strings and information from system

static BOOLEAN g_HypervisorHiderActive = FALSE;

// Hypervisor signatures to detect and hide
static const CHAR* g_HypervisorSignatures[] = {
    "Hv#1",
    "Microsoft Hv",
    "VMwareVMware",
    "VBoxVBoxVBox",
    "KVMKVMKVM",
    "XenVMMXenVMM",
    NULL
};

// Check and mask CPUID hypervisor information
static VOID MaskHypervisorCPUID(void) {
    int cpuInfo[4];
    
    // Check CPUID leaf 0x1 for hypervisor bit
    __cpuid(cpuInfo, 1);
    
    if (cpuInfo[2] & (1 << 31)) {
        VmLog("[HvHide] Hypervisor present bit detected in CPUID leaf 0x1");
        // Note: Cannot directly modify CPUID results from kernel driver
        // This requires hypervisor-level interception
    }
    
    // Check CPUID leaf 0x40000000 for hypervisor vendor
    __cpuid(cpuInfo, 0x40000000);
    
    if (cpuInfo[0] >= 0x40000000 && cpuInfo[0] <= 0x40000010) {
        char vendor[13] = {0};
        *(int*)&vendor[0] = cpuInfo[1];
        *(int*)&vendor[4] = cpuInfo[2];
        *(int*)&vendor[8] = cpuInfo[3];
        
        VmLog("[HvHide] Hypervisor vendor detected: %s", vendor);
        
        for (SIZE_T i = 0; g_HypervisorSignatures[i] != NULL; i++) {
            if (RtlCompareMemory(vendor, g_HypervisorSignatures[i], 12) == 12) {
                VmLog("[HvHide] Known hypervisor signature found: %s", g_HypervisorSignatures[i]);
            }
        }
    }
}

// Clean hypervisor strings from registry
static VOID CleanHypervisorRegistry(void) {
    // Registry keys that may contain hypervisor information:
    // HKLM\HARDWARE\DESCRIPTION\System\SystemBiosVersion
    // HKLM\HARDWARE\DESCRIPTION\System\VideoBiosVersion
    
    // This is handled by RegistryCleaner module
    VmLog("[HvHide] Hypervisor registry cleaning delegated to RegistryCleaner");
}

NTSTATUS HypervisorHiderInitialize(void) {
#if !ENABLE_HYPERVISOR_HIDING
    return STATUS_SUCCESS;
#endif
    
    VmLog("[HvHide] Initializing hypervisor hider module...");
    
    // Check for hypervisor presence
    MaskHypervisorCPUID();
    
    // Clean registry artifacts
    CleanHypervisorRegistry();
    
    VmLog("[HvHide] Hypervisor hider initialized");
    VmLog("[HvHide] Note: CPUID masking requires hypervisor-level interception");
    VmLog("[HvHide] Use VMX configuration: hypervisor.cpuid.v0 = FALSE");
    
    g_HypervisorHiderActive = TRUE;
    return STATUS_SUCCESS;
}

VOID HypervisorHiderCleanup(void) {
    if (g_HypervisorHiderActive) {
        VmLog("[HvHide] Cleaning up hypervisor hider module...");
        g_HypervisorHiderActive = FALSE;
        VmLog("[HvHide] Cleanup complete");
    }
}

BOOLEAN HypervisorHiderIsActive(void) {
    return g_HypervisorHiderActive;
}
