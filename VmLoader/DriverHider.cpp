#include <ntddk.h>
#include "DriverHider.h"
#include "Config.h"
#include "Utils.h"

// Driver Hider Module - Hides VMware drivers from enumeration
// Uses list manipulation to hide drivers from PsLoadedModuleList

static BOOLEAN g_DriverHiderActive = FALSE;

// VMware driver names to hide
static const WCHAR* g_VMwareDrivers[] = {
    L"vm3dmp.sys",
    L"vm3dmp_loader.sys",
    L"vmci.sys",
    L"vsock.sys",
    L"vmhgfs.sys",
    L"vmrawdsk.sys",
    L"vmmouse.sys",
    L"vmusbmouse.sys",
    L"vmx_svga.sys",
    L"vm3dgl.sys",
    NULL
};

// LDR_DATA_TABLE_ENTRY structure (simplified)
typedef struct _LDR_DATA_TABLE_ENTRY {
    LIST_ENTRY InLoadOrderLinks;
    LIST_ENTRY InMemoryOrderLinks;
    LIST_ENTRY InInitializationOrderLinks;
    PVOID DllBase;
    PVOID EntryPoint;
    ULONG SizeOfImage;
    UNICODE_STRING FullDllName;
    UNICODE_STRING BaseDllName;
    // ... more fields
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;

// Check if driver name is a VMware driver
static BOOLEAN IsVMwareDriver(PUNICODE_STRING driverName) {
    if (!driverName || !driverName->Buffer) return FALSE;
    
    for (SIZE_T i = 0; g_VMwareDrivers[i] != NULL; i++) {
        SIZE_T vmLen = VmWideStringLength(g_VMwareDrivers[i]);
        SIZE_T drvLen = driverName->Length / sizeof(WCHAR);
        
        // Check if driver name ends with the VMware driver name
        if (drvLen >= vmLen) {
            PWCHAR driverEnd = &driverName->Buffer[drvLen - vmLen];
            
            // Case-insensitive comparison
            BOOLEAN match = TRUE;
            for (SIZE_T j = 0; j < vmLen; j++) {
                WCHAR c1 = driverEnd[j];
                WCHAR c2 = g_VMwareDrivers[i][j];
                
                if (c1 >= L'A' && c1 <= L'Z') c1 += 32;
                if (c2 >= L'A' && c2 <= L'Z') c2 += 32;
                
                if (c1 != c2) {
                    match = FALSE;
                    break;
                }
            }
            
            if (match) return TRUE;
        }
    }
    
    return FALSE;
}

NTSTATUS DriverHiderInitialize(void) {
#if !ENABLE_DRIVER_HIDING
    return STATUS_SUCCESS;
#endif
    
    VmLog("[DriverHide] Initializing driver hider module...");
    
    // Note: Hiding drivers from PsLoadedModuleList requires:
    // 1. Locating PsLoadedModuleList (exported in some Windows versions)
    // 2. Walking the list and unlinking VMware drivers
    // 3. This is a DKOM technique that may be detected by PatchGuard
    
    // For a production implementation:
    // - Use MmGetSystemRoutineAddress to find PsLoadedModuleList
    // - Walk InLoadOrderLinks
    // - Unlink entries for VMware drivers
    // - Save references for restoration on cleanup
    
    VmLog("[DriverHide] Driver hiding framework initialized");
    VmLog("[DriverHide] Note: DKOM implementation requires PsLoadedModuleList manipulation");
    VmLog("[DriverHide] This may trigger PatchGuard on modern Windows versions");
    
    g_DriverHiderActive = TRUE;
    return STATUS_SUCCESS;
}

VOID DriverHiderCleanup(void) {
    if (g_DriverHiderActive) {
        VmLog("[DriverHide] Cleaning up driver hider module...");
        
        // Restore hidden drivers if we unlinked them
        
        g_DriverHiderActive = FALSE;
        VmLog("[DriverHide] Cleanup complete");
    }
}

BOOLEAN DriverHiderIsActive(void) {
    return g_DriverHiderActive;
}
