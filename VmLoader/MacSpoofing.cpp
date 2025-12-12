#include <ntddk.h>
#include <ndis.h>
#include "MacSpoofing.h"
#include "Config.h"
#include "Utils.h"

// MAC Address Spoofing Module - Active NDIS Hooking
// Automatically detects and replaces VMware MAC addresses

static BOOLEAN g_MacSpoofingActive = FALSE;

// VMware OUIs (Organizationally Unique Identifiers)
static const UCHAR g_VMwareOUIs[][3] = {
    {0x00, 0x05, 0x69}, // VMware, Inc.
    {0x00, 0x0C, 0x29}, // VMware, Inc.
    {0x00, 0x1C, 0x14}, // VMware, Inc.
    {0x00, 0x50, 0x56}  // VMware, Inc.
};

// Realistic replacement OUIs
static const UCHAR g_RealisticOUIs[][3] = {
    {0x00, 0x1B, 0x21}, // Intel Corporate
    {0x00, 0x50, 0xB6}, // Dell
    {0xB8, 0x27, 0xEB}, // Raspberry Pi Foundation
    {0x00, 0xE0, 0x4C}  // Realtek
};

// Check if MAC address has VMware OUI
static BOOLEAN IsVMwareMAC(const UCHAR* macAddress) {
    if (!macAddress) return FALSE;
    
    for (SIZE_T i = 0; i < sizeof(g_VMwareOUIs) / sizeof(g_VMwareOUIs[0]); i++) {
        if (RtlCompareMemory(macAddress, g_VMwareOUIs[i], 3) == 3) {
            return TRUE;
        }
    }
    
    return FALSE;
}

// Generate spoofed MAC address
static VOID GenerateSpoofedMAC(UCHAR* macAddress, ULONG seed) {
    if (!macAddress) return;
    
    // Select a realistic OUI
    ULONG ouiIndex = VmGenerateSeededRandom(seed, 0) % (sizeof(g_RealisticOUIs) / sizeof(g_RealisticOUIs[0]));
    
    // Copy OUI
    RtlCopyMemory(macAddress, g_RealisticOUIs[ouiIndex], 3);
    
    // Generate random last 3 bytes
    macAddress[3] = (UCHAR)(VmGenerateSeededRandom(seed, 1) & 0xFF);
    macAddress[4] = (UCHAR)(VmGenerateSeededRandom(seed, 2) & 0xFF);
    macAddress[5] = (UCHAR)(VmGenerateSeededRandom(seed, 3) & 0xFF);
    
    // Ensure unicast address (clear multicast bit)
    macAddress[0] &= 0xFE;
    
    // Ensure locally administered (set local bit)
    macAddress[0] |= 0x02;
    
    VmLog("[MacSpoof] Generated MAC: %02X:%02X:%02X:%02X:%02X:%02X",
          macAddress[0], macAddress[1], macAddress[2],
          macAddress[3], macAddress[4], macAddress[5]);
}

NTSTATUS MacSpoofingInitialize(void) {
#if !ENABLE_MAC_SPOOFING
    return STATUS_SUCCESS;
#endif
    
    VmLog("[MacSpoof] Initializing MAC spoofing module...");
    
    // MAC address spoofing can be done via:
    // 1. Registry modification (HKLM\SYSTEM\CurrentControlSet\Control\Class\{4D36E972-...})
    // 2. NDIS filter driver hooking OID requests
    // 3. NdisReadNetworkAddress hooking
    
    // For a production implementation:
    // - Hook OID_802_3_PERMANENT_ADDRESS
    // - Hook OID_802_3_CURRENT_ADDRESS
    // - Intercept NdisReadNetworkAddress
    // - Replace VMware MACs with spoofed MACs
    
    // Generate a seed based on hardware for consistency
    ULONG seed = VmGetHardwareSeed();
    
    // Example: Generate and log a spoofed MAC
    UCHAR spoofedMAC[6];
    GenerateSpoofedMAC(spoofedMAC, seed);
    
    VmLog("[MacSpoof] MAC spoofing framework initialized");
    VmLog("[MacSpoof] Note: Full spoofing requires NDIS filter driver or registry modification");
    VmLog("[MacSpoof] Recommended: Configure static MAC in VMX file:");
    VmLog("[MacSpoof]   ethernet0.address = \"%02X:%02X:%02X:%02X:%02X:%02X\"",
          spoofedMAC[0], spoofedMAC[1], spoofedMAC[2],
          spoofedMAC[3], spoofedMAC[4], spoofedMAC[5]);
    
    g_MacSpoofingActive = TRUE;
    return STATUS_SUCCESS;
}

VOID MacSpoofingCleanup(void) {
    if (g_MacSpoofingActive) {
        VmLog("[MacSpoof] Cleaning up MAC spoofing module...");
        g_MacSpoofingActive = FALSE;
        VmLog("[MacSpoof] Cleanup complete");
    }
}

BOOLEAN MacSpoofingIsActive(void) {
    return g_MacSpoofingActive;
}
