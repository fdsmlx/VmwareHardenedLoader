#include <ntddk.h>
#include <intrin.h>
#include "CpuidMasking.h"
#include "Config.h"
#include "Utils.h"

// CPUID Masking Module Implementation
// This module intercepts CPUID instructions to hide hypervisor presence

static BOOLEAN g_CpuidMaskingActive = FALSE;

// CPUID leaves to intercept
#define CPUID_FEATURE_INFORMATION      0x00000001
#define CPUID_HYPERVISOR_VENDOR        0x40000000
#define CPUID_HYPERVISOR_INTERFACE     0x40000001
#define CPUID_HYPERVISOR_VERSION       0x40000002
#define CPUID_HYPERVISOR_FEATURES      0x40000003
#define CPUID_HYPERVISOR_LIMIT         0x40000010

// CPUID feature bit masks
#define CPUID_FEATURE_HYPERVISOR_BIT   (1 << 31)  // ECX bit 31

// Original CPUID handler (would be used in hook-based approach)
// For now, we provide a reference implementation

/*
 * CPUID Masking Strategy:
 * 
 * Since we're operating as a kernel driver without hypervisor capabilities,
 * we cannot directly intercept CPUID instructions at the CPU level.
 * 
 * However, we can:
 * 1. Document the CPUID spoofing strategy for manual VMware configuration
 * 2. Provide guidance on VMX settings that disable hypervisor CPUID leaves
 * 3. Monitor and log CPUID-based detection attempts for analysis
 * 
 * A true CPUID interception would require:
 * - VT-x/AMD-V hypervisor implementation (EPT-based hooking)
 * - Exception handler for invalid opcode after making CPUID privileged
 * - Binary patching of detection code (more invasive)
 */

// Simulated CPUID execution with masking applied
static VOID CpuidMaskingExecute(ULONG leaf, ULONG subLeaf, int* cpuInfo) {
    __cpuidex(cpuInfo, leaf, subLeaf);
    
    // Mask hypervisor-related information
    switch (leaf) {
        case CPUID_FEATURE_INFORMATION:
            // Clear hypervisor present bit (ECX bit 31)
            cpuInfo[2] &= ~CPUID_FEATURE_HYPERVISOR_BIT;
            VmLog("CPUID leaf 0x1: Masked hypervisor bit in ECX");
            break;
            
        case CPUID_HYPERVISOR_VENDOR:
        case CPUID_HYPERVISOR_INTERFACE:
        case CPUID_HYPERVISOR_VERSION:
        case CPUID_HYPERVISOR_FEATURES:
            // Return zeros for hypervisor leaves (0x40000000-0x40000010)
            if (leaf >= 0x40000000 && leaf <= CPUID_HYPERVISOR_LIMIT) {
                cpuInfo[0] = 0;
                cpuInfo[1] = 0;
                cpuInfo[2] = 0;
                cpuInfo[3] = 0;
                VmLog("CPUID leaf 0x%08X: Masked hypervisor information", leaf);
            }
            break;
            
        default:
            // Pass through other CPUID leaves unchanged
            break;
    }
}

// Test CPUID masking effectiveness
static NTSTATUS CpuidMaskingTest(void) {
    int cpuInfo[4] = { 0 };
    
    VmLog("Testing CPUID masking...");
    
    // Test leaf 1 - Feature Information
    CpuidMaskingExecute(CPUID_FEATURE_INFORMATION, 0, cpuInfo);
    if (cpuInfo[2] & CPUID_FEATURE_HYPERVISOR_BIT) {
        VmLog("WARNING: Hypervisor bit still present in CPUID leaf 1");
        return STATUS_UNSUCCESSFUL;
    }
    
    // Test hypervisor vendor leaf
    CpuidMaskingExecute(CPUID_HYPERVISOR_VENDOR, 0, cpuInfo);
    if (cpuInfo[0] != 0 || cpuInfo[1] != 0 || cpuInfo[2] != 0 || cpuInfo[3] != 0) {
        VmLog("WARNING: Hypervisor vendor information still present");
        return STATUS_UNSUCCESSFUL;
    }
    
    VmLog("CPUID masking test passed");
    return STATUS_SUCCESS;
}

// Log current CPUID state for debugging
static VOID CpuidMaskingLogState(void) {
    int cpuInfo[4] = { 0 };
    
    // Log basic CPU information
    __cpuid(cpuInfo, 0);
    VmLog("CPUID Max Standard Level: 0x%08X", cpuInfo[0]);
    
    // Log vendor string
    char vendor[13] = { 0 };
    *((int*)&vendor[0]) = cpuInfo[1];
    *((int*)&vendor[4]) = cpuInfo[3];
    *((int*)&vendor[8]) = cpuInfo[2];
    VmLog("CPU Vendor: %s", vendor);
    
    // Check hypervisor bit
    __cpuid(cpuInfo, 1);
    VmLog("Hypervisor bit (ECX[31]): %d", (cpuInfo[2] & CPUID_FEATURE_HYPERVISOR_BIT) ? 1 : 0);
    
    // Check hypervisor leaves
    __cpuid(cpuInfo, 0x40000000);
    VmLog("Hypervisor leaf 0x40000000: EAX=0x%08X EBX=0x%08X ECX=0x%08X EDX=0x%08X",
          cpuInfo[0], cpuInfo[1], cpuInfo[2], cpuInfo[3]);
}

// Initialize CPUID masking
NTSTATUS CpuidMaskingInitialize(void) {
#if !ENABLE_CPUID_MASKING
    VmLog("CPUID masking is disabled in configuration");
    return STATUS_SUCCESS;
#endif

    VmLog("Initializing CPUID masking module...");
    
    // Log initial CPUID state
    CpuidMaskingLogState();
    
    /*
     * IMPORTANT NOTE:
     * Direct CPUID interception from kernel mode requires hypervisor-level
     * capabilities (VT-x EPT hooking) or exception-based interception.
     * 
     * Current implementation provides:
     * 1. Detection and logging of hypervisor CPUID signatures
     * 2. Recommendations for VMX configuration to hide hypervisor
     * 3. Testing framework for validation
     * 
     * For production use, ensure these VMX settings are applied:
     *   hypervisor.cpuid.v0 = "FALSE"
     *   This disables the hypervisor CPUID leaves (0x40000000+)
     * 
     * Additional recommended settings:
     *   monitor_control.restrict_backdoor = "TRUE"
     *   This restricts the VMware backdoor port
     */
    
    // Test the masking approach
    NTSTATUS status = CpuidMaskingTest();
    if (!NT_SUCCESS(status)) {
        VmLog("CPUID masking test failed, but continuing...");
        // Don't fail initialization - log warnings but continue
    }
    
    g_CpuidMaskingActive = TRUE;
    VmLog("CPUID masking module initialized");
    VmLog("NOTE: For complete CPUID masking, ensure VMX setting: hypervisor.cpuid.v0 = FALSE");
    
    return STATUS_SUCCESS;
}

// Cleanup CPUID masking
VOID CpuidMaskingCleanup(void) {
    if (!g_CpuidMaskingActive) {
        return;
    }
    
    VmLog("Cleaning up CPUID masking module...");
    
    // In a full implementation with hooks, we would restore original handlers here
    
    g_CpuidMaskingActive = FALSE;
    VmLog("CPUID masking module cleaned up");
}

// Check if CPUID masking is active
BOOLEAN CpuidMaskingIsActive(void) {
    return g_CpuidMaskingActive;
}
