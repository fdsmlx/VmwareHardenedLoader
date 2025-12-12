#include <ntddk.h>
#include "DiskSpoofing.h"
#include "Config.h"
#include "Utils.h"

static BOOLEAN g_DiskSpoofingActive = FALSE;

NTSTATUS DiskSpoofingInitialize(void) {
#if !ENABLE_DISK_SPOOFING
    return STATUS_SUCCESS;
#endif
    VmLog("Initializing disk spoofing module...");
    g_DiskSpoofingActive = TRUE;
    return STATUS_SUCCESS;
}

VOID DiskSpoofingCleanup(void) {
    if (g_DiskSpoofingActive) {
        VmLog("Cleaning up disk spoofing module...");
        g_DiskSpoofingActive = FALSE;
    }
}

BOOLEAN DiskSpoofingIsActive(void) {
    return g_DiskSpoofingActive;
}
