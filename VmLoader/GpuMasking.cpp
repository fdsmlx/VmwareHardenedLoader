#include <ntddk.h>
#include "GpuMasking.h"
#include "Config.h"
#include "Utils.h"

static BOOLEAN g_GpuMaskingActive = FALSE;

NTSTATUS GpuMaskingInitialize(void) {
#if !ENABLE_GPU_MASKING
    return STATUS_SUCCESS;
#endif
    VmLog("Initializing GPU masking module...");
    g_GpuMaskingActive = TRUE;
    return STATUS_SUCCESS;
}

VOID GpuMaskingCleanup(void) {
    if (g_GpuMaskingActive) {
        VmLog("Cleaning up GPU masking module...");
        g_GpuMaskingActive = FALSE;
    }
}

BOOLEAN GpuMaskingIsActive(void) {
    return g_GpuMaskingActive;
}
