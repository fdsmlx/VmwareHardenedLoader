#include <ntddk.h>
#include "PciSpoofing.h"
#include "Config.h"
#include "Utils.h"

static BOOLEAN g_PciSpoofingActive = FALSE;

NTSTATUS PciSpoofingInitialize(void) {
#if !ENABLE_PCI_SPOOFING
    return STATUS_SUCCESS;
#endif
    VmLog("Initializing PCI spoofing module...");
    g_PciSpoofingActive = TRUE;
    return STATUS_SUCCESS;
}

VOID PciSpoofingCleanup(void) {
    if (g_PciSpoofingActive) {
        VmLog("Cleaning up PCI spoofing module...");
        g_PciSpoofingActive = FALSE;
    }
}

BOOLEAN PciSpoofingIsActive(void) {
    return g_PciSpoofingActive;
}
