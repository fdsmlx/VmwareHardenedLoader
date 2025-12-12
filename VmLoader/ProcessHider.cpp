#include <ntddk.h>
#include "ProcessHider.h"
#include "Config.h"
#include "Utils.h"

static BOOLEAN g_ProcessHiderActive = FALSE;

NTSTATUS ProcessHiderInitialize(void) {
#if !ENABLE_PROCESS_HIDING
    return STATUS_SUCCESS;
#endif
    VmLog("Initializing process hider module...");
    g_ProcessHiderActive = TRUE;
    return STATUS_SUCCESS;
}

VOID ProcessHiderCleanup(void) {
    if (g_ProcessHiderActive) {
        VmLog("Cleaning up process hider module...");
        g_ProcessHiderActive = FALSE;
    }
}

BOOLEAN ProcessHiderIsActive(void) {
    return g_ProcessHiderActive;
}
