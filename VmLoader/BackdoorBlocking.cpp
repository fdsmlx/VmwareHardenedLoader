#include <ntddk.h>
#include "BackdoorBlocking.h"
#include "Config.h"
#include "Utils.h"

// VMware Backdoor Blocking Module Implementation
// This module provides detection and recommendations for blocking VMware backdoor ports

static BOOLEAN g_BackdoorBlockingActive = FALSE;

/*
 * VMware Backdoor Communication:
 * 
 * VMware provides a backdoor interface for guest-to-host communication via I/O ports:
 * - Port 0x5658 (VX): Standard backdoor port
 * - Port 0x5659 (VY): High-bandwidth backdoor port
 * 
 * The backdoor uses a magic number: 0x564D5868 ("VMXh")
 * 
 * Backdoor Command Format:
 * - EAX: Magic number (0x564D5868)
 * - EBX: Command-specific parameter
 * - ECX: Command number
 * - EDX: Port number (0x5658 or 0x5659)
 * 
 * Common Commands:
 * - CMD_GETVERSION (10): Get VMware version
 * - CMD_GETHZ (45): Get CPU frequency
 * - CMD_GETTIME (23): Get host time
 * 
 * Detection Method:
 * - Software can attempt to communicate via these ports
 * - If successful, it indicates VMware presence
 * 
 * Blocking Strategy:
 * - Kernel-mode drivers cannot directly intercept IN/OUT instructions
 * - Requires hypervisor-level interception or I/O port filtering
 * - Best approach: Disable in VMX configuration
 */

// VMware backdoor command definitions
#define BACKDOOR_CMD_GETVERSION     10
#define BACKDOOR_CMD_GETHZ          45
#define BACKDOOR_CMD_GETTIME        23
#define BACKDOOR_CMD_GETTIMEFULL    23

// Test if backdoor is accessible (for detection purposes)
static BOOLEAN BackdoorTestAccess(void) {
    // We cannot safely test backdoor access from kernel mode without risking
    // system instability. Instead, we rely on configuration validation.
    
    VmLog("Backdoor access testing skipped (requires hypervisor intercept)");
    return FALSE;
}

// Log backdoor blocking recommendations
static VOID BackdoorLogRecommendations(void) {
    VmLog("=== VMware Backdoor Blocking Recommendations ===");
    VmLog("Add these critical settings to your .vmx file:");
    VmLog("");
    VmLog("# Disable VMware backdoor communication");
    VmLog("  isolation.tools.getPtrLocation.disable = \"TRUE\"");
    VmLog("  isolation.tools.setPtrLocation.disable = \"TRUE\"");
    VmLog("  isolation.tools.setVersion.disable = \"TRUE\"");
    VmLog("  isolation.tools.getVersion.disable = \"TRUE\"");
    VmLog("  monitor_control.restrict_backdoor = \"TRUE\"");
    VmLog("");
    VmLog("# Disable VMware guest tools features");
    VmLog("  isolation.tools.ghi.autologon.disable = \"TRUE\"");
    VmLog("  isolation.tools.hgfs.disable = \"TRUE\"");
    VmLog("  isolation.tools.memSchedFakeSampleStats.disable = \"TRUE\"");
    VmLog("  isolation.tools.ghi.launchmenu.change = \"TRUE\"");
    VmLog("  isolation.tools.ghi.protocolhandler.info = \"TRUE\"");
    VmLog("  isolation.tools.getCreds.disable = \"TRUE\"");
    VmLog("  isolation.tools.ghi.trayicon.disable = \"TRUE\"");
    VmLog("  isolation.tools.unity.disable = \"TRUE\"");
    VmLog("  isolation.tools.unityInterlockOperation.disable = \"TRUE\"");
    VmLog("  isolation.tools.unity.push.update.disable = \"TRUE\"");
    VmLog("  isolation.tools.unity.taskbar.disable = \"TRUE\"");
    VmLog("  isolation.tools.unityActive.disable = \"TRUE\"");
    VmLog("  isolation.tools.vmxDnDVersionGet.disable = \"TRUE\"");
    VmLog("  isolation.tools.guestDnDVersionSet.disable = \"TRUE\"");
    VmLog("");
    VmLog("# Disable drag and drop");
    VmLog("  isolation.tools.dnd.disable = \"TRUE\"");
    VmLog("  isolation.tools.copy.disable = \"TRUE\"");
    VmLog("  isolation.tools.paste.disable = \"TRUE\"");
    VmLog("");
    VmLog("IMPORTANT: Do NOT install VMware Tools!");
    VmLog("VMware Tools will enable backdoor communication and expose VM presence.");
}

// Analyze system for VMware backdoor indicators
static NTSTATUS BackdoorAnalyzeSystem(void) {
    VmLog("Analyzing system for VMware backdoor indicators...");
    
    // Check for VMware-related registry keys (simplified check)
    // In a full implementation, we would scan registry for VMware indicators
    
    /*
     * Registry keys to check:
     * - HKLM\SOFTWARE\VMware, Inc.
     * - HKLM\SYSTEM\CurrentControlSet\Services\vm*
     * - HKLM\SYSTEM\CurrentControlSet\Services\VMware*
     */
    
    VmLog("System analysis: No direct backdoor port interception available");
    VmLog("Rely on VMX configuration for backdoor blocking");
    
    return STATUS_SUCCESS;
}

// Validate VMX configuration (conceptual - cannot actually read .vmx from guest)
static VOID BackdoorValidateConfiguration(void) {
    VmLog("=== Backdoor Configuration Validation ===");
    VmLog("Please manually verify your .vmx file contains the recommended settings.");
    VmLog("Without these settings, VMware backdoor detection will succeed.");
    VmLog("");
    VmLog("Key settings to verify:");
    VmLog("  1. monitor_control.restrict_backdoor = \"TRUE\"");
    VmLog("  2. isolation.tools.* disable settings");
    VmLog("  3. No VMware Tools installed in guest");
}

// Initialize backdoor blocking
NTSTATUS BackdoorBlockingInitialize(void) {
#if !ENABLE_BACKDOOR_BLOCKING
    VmLog("Backdoor blocking is disabled in configuration");
    return STATUS_SUCCESS;
#endif

    VmLog("Initializing backdoor blocking module...");
    
    // Analyze system for backdoor indicators
    NTSTATUS status = BackdoorAnalyzeSystem();
    if (!NT_SUCCESS(status)) {
        VmLog("WARNING: System analysis failed");
    }
    
    // Log blocking recommendations
    BackdoorLogRecommendations();
    
    // Validate configuration
    BackdoorValidateConfiguration();
    
    /*
     * IMPORTANT NOTE:
     * Direct I/O port interception (IN/OUT instructions) requires:
     * - Hypervisor-level I/O port bitmap configuration
     * - VT-x I/O port filtering
     * - Exception handling for port access
     * 
     * Current implementation provides:
     * 1. Detection guidance for VMware backdoor presence
     * 2. Comprehensive VMX configuration recommendations
     * 3. System analysis for VMware indicators
     * 
     * For production use:
     * - Apply all recommended VMX settings
     * - Do NOT install VMware Tools
     * - Use the restricted backdoor setting
     * 
     * Advanced mitigation requires:
     * - Hypervisor-based I/O port interception
     * - VMX I/O bitmap configuration
     * - Binary patching of backdoor detection code
     */
    
    g_BackdoorBlockingActive = TRUE;
    VmLog("Backdoor blocking module initialized");
    VmLog("NOTE: Backdoor blocking relies on VMX configuration settings");
    
    return STATUS_SUCCESS;
}

// Cleanup backdoor blocking
VOID BackdoorBlockingCleanup(void) {
    if (!g_BackdoorBlockingActive) {
        return;
    }
    
    VmLog("Cleaning up backdoor blocking module...");
    
    g_BackdoorBlockingActive = FALSE;
    VmLog("Backdoor blocking module cleaned up");
}

// Check if backdoor blocking is active
BOOLEAN BackdoorBlockingIsActive(void) {
    return g_BackdoorBlockingActive;
}
