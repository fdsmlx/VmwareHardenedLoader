#include <ntddk.h>
#include "MacSpoofing.h"
#include "Config.h"
#include "Utils.h"

static BOOLEAN g_MacSpoofingActive = FALSE;

NTSTATUS MacSpoofingInitialize(void) {
#if !ENABLE_MAC_SPOOFING
    return STATUS_SUCCESS;
#endif
    VmLog("Initializing MAC spoofing module...");
    g_MacSpoofingActive = TRUE;
    return STATUS_SUCCESS;
}

VOID MacSpoofingCleanup(void) {
    if (g_MacSpoofingActive) {
        VmLog("Cleaning up MAC spoofing module...");
        g_MacSpoofingActive = FALSE;
    }
}

BOOLEAN MacSpoofingIsActive(void) {
    return g_MacSpoofingActive;
}
