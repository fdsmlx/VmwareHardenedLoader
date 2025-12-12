#include <ntddk.h>
#include <intrin.h>
#include "TimingNormalization.h"
#include "Config.h"
#include "Utils.h"

// Timing Normalization Module Implementation
// This module aims to normalize timing behavior to avoid VM detection via timing attacks

static BOOLEAN g_TimingNormalizationActive = FALSE;
static LARGE_INTEGER g_PerformanceFrequency = { 0 };
static ULONG64 g_BaselineTsc = 0;
static LARGE_INTEGER g_BaselineQpc = { 0 };

/*
 * Timing Attack Detection Methods:
 * 
 * 1. RDTSC/RDTSCP instruction timing
 *    - VMs typically show larger deltas due to hypervisor overhead
 *    - Detection: Measure time for simple operations
 * 
 * 2. QueryPerformanceCounter variations
 *    - Compare RDTSC with QPC for inconsistencies
 * 
 * 3. Instruction execution timing
 *    - CPUID, VMCALL instructions take longer in VMs
 * 
 * Mitigation Strategy:
 * - Cannot directly hook RDTSC (requires hypervisor or CPU microcode)
 * - Can provide timing correlation analysis
 * - Can recommend VMX settings for timing normalization
 */

// Calculate expected TSC value based on QPC
static ULONG64 TimingGetExpectedTsc(void) {
    LARGE_INTEGER currentQpc;
    KeQueryPerformanceCounter(&currentQpc);
    
    // Check for zero frequency to avoid division by zero
    if (g_PerformanceFrequency.QuadPart == 0) {
        VmLog("WARNING: Performance frequency is zero");
        return g_BaselineTsc;
    }
    
    // Calculate elapsed time in ticks
    ULONG64 elapsedTicks = currentQpc.QuadPart - g_BaselineQpc.QuadPart;
    
    // Convert to TSC cycles (approximate)
    // TSC frequency ≈ CPU frequency
    ULONG64 elapsedSeconds = elapsedTicks / g_PerformanceFrequency.QuadPart;
    ULONG64 expectedTsc = g_BaselineTsc + (elapsedSeconds * RDTSC_BASE_FREQUENCY);
    
    return expectedTsc;
}

// Measure RDTSC overhead (for detection analysis)
static ULONG64 TimingMeasureRdtscOverhead(void) {
    ULONG64 start, end;
    ULONG iterations = 1000;
    ULONG64 totalOverhead = 0;
    
    for (ULONG i = 0; i < iterations; i++) {
        start = __rdtsc();
        end = __rdtsc();
        totalOverhead += (end - start);
    }
    
    return totalOverhead / iterations;
}

// Test timing behavior for VM detection signatures
static NTSTATUS TimingTestBehavior(void) {
    VmLog("Testing timing behavior...");
    
    // Test 1: RDTSC overhead
    ULONG64 rdtscOverhead = TimingMeasureRdtscOverhead();
    VmLog("Average RDTSC overhead: %llu cycles", rdtscOverhead);
    
    // Typical values:
    // - Physical CPU: 20-40 cycles
    // - VMware: 100-300 cycles (without timing normalization)
    if (rdtscOverhead > 80) {
        VmLog("WARNING: High RDTSC overhead detected (possible VM signature)");
    }
    
    // Test 2: CPUID timing
    ULONG64 cpuidStart = __rdtsc();
    int cpuInfo[4];
    __cpuid(cpuInfo, 0);
    ULONG64 cpuidEnd = __rdtsc();
    ULONG64 cpuidCycles = cpuidEnd - cpuidStart;
    VmLog("CPUID execution time: %llu cycles", cpuidCycles);
    
    // Typical values:
    // - Physical CPU: 100-300 cycles
    // - VMware: 1000-5000 cycles
    if (cpuidCycles > 800) {
        VmLog("WARNING: High CPUID execution time (possible VM signature)");
    }
    
    // Test 3: Simple instruction timing
    ULONG64 loopStart = __rdtsc();
    volatile int dummy = 0;
    for (int i = 0; i < 1000; i++) {
        dummy += i;
    }
    ULONG64 loopEnd = __rdtsc();
    ULONG64 loopCycles = loopEnd - loopStart;
    VmLog("1000 iterations loop time: %llu cycles", loopCycles);
    
    // Test 4: TSC vs QPC correlation
    ULONG64 tsc1 = __rdtsc();
    LARGE_INTEGER qpc1;
    KeQueryPerformanceCounter(&qpc1);
    
    // Small delay
    LARGE_INTEGER delay;
    delay.QuadPart = -10000; // 1ms
    KeDelayExecutionThread(KernelMode, FALSE, &delay);
    
    LARGE_INTEGER qpc2;
    KeQueryPerformanceCounter(&qpc2);
    ULONG64 tsc2 = __rdtsc();
    
    ULONG64 tscDelta = tsc2 - tsc1;
    ULONG64 qpcDelta = qpc2.QuadPart - qpc1.QuadPart;
    
    VmLog("TSC delta: %llu, QPC delta: %llu", tscDelta, qpcDelta);
    
    return STATUS_SUCCESS;
}

// Log timing recommendations
static VOID TimingLogRecommendations(void) {
    VmLog("=== Timing Normalization Recommendations ===");
    VmLog("Add these settings to your .vmx file:");
    VmLog("  monitor_control.disable_directexec = \"TRUE\"");
    VmLog("  monitor_control.disable_chksimd = \"TRUE\"");
    VmLog("  monitor_control.disable_ntreloc = \"TRUE\"");
    VmLog("  monitor_control.disable_selfmod = \"TRUE\"");
    VmLog("  monitor_control.disable_reloc = \"TRUE\"");
    VmLog("  monitor_control.disable_btinout = \"TRUE\"");
    VmLog("  monitor_control.disable_btmemspace = \"TRUE\"");
    VmLog("  monitor_control.disable_btpriv = \"TRUE\"");
    VmLog("  monitor_control.disable_btseg = \"TRUE\"");
    VmLog("");
    VmLog("For improved timing behavior:");
    VmLog("  monitor_control.virtual_rdtsc = \"TRUE\"");
    VmLog("  monitor_control.virtual_rdtsc_offset = \"0\"");
}

// Initialize timing normalization
NTSTATUS TimingNormalizationInitialize(void) {
#if !ENABLE_TIMING_NORMALIZATION
    VmLog("Timing normalization is disabled in configuration");
    return STATUS_SUCCESS;
#endif

    VmLog("Initializing timing normalization module...");
    
    // Get performance counter frequency
    KeQueryPerformanceCounter(&g_PerformanceFrequency);
    
    // Validate frequency is non-zero
    if (g_PerformanceFrequency.QuadPart == 0) {
        VmLog("ERROR: Performance counter frequency is zero!");
        return STATUS_UNSUCCESSFUL;
    }
    
    VmLog("Performance counter frequency: %llu Hz", g_PerformanceFrequency.QuadPart);
    
    // Establish baseline measurements
    g_BaselineTsc = __rdtsc();
    KeQueryPerformanceCounter(&g_BaselineQpc);
    VmLog("Baseline TSC: %llu", g_BaselineTsc);
    
    // Run timing tests
    NTSTATUS status = TimingTestBehavior();
    if (!NT_SUCCESS(status)) {
        VmLog("WARNING: Timing tests detected VM signatures");
    }
    
    // Log recommendations
    TimingLogRecommendations();
    
    /*
     * IMPORTANT NOTE:
     * Direct RDTSC interception requires hypervisor-level capabilities.
     * 
     * Current implementation provides:
     * 1. Timing behavior analysis and logging
     * 2. Detection of VM timing signatures
     * 3. Recommendations for VMX configuration
     * 
     * For production use, apply the recommended VMX settings above.
     * 
     * Advanced mitigation would require:
     * - Hypervisor-based RDTSC interception (EPT)
     * - Hardware TSC offsetting (available in newer CPUs)
     * - Binary patching of timing-based detection code
     */
    
    g_TimingNormalizationActive = TRUE;
    VmLog("Timing normalization module initialized");
    
    return STATUS_SUCCESS;
}

// Cleanup timing normalization
VOID TimingNormalizationCleanup(void) {
    if (!g_TimingNormalizationActive) {
        return;
    }
    
    VmLog("Cleaning up timing normalization module...");
    
    g_TimingNormalizationActive = FALSE;
    VmLog("Timing normalization module cleaned up");
}

// Check if timing normalization is active
BOOLEAN TimingNormalizationIsActive(void) {
    return g_TimingNormalizationActive;
}
