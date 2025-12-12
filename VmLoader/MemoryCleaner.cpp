#include <ntddk.h>
#include "MemoryCleaner.h"
#include "Config.h"
#include "Utils.h"

// External kernel variables
extern POBJECT_TYPE *PsThreadType;

// Memory Cleaner Module - Scans and cleans VMware artifacts from kernel memory
// Periodically scans NonPagedPool for VMware strings and cleans them

static BOOLEAN g_MemoryCleanerActive = FALSE;
static PVOID g_CleanerThread = NULL;
static KEVENT g_StopEvent;

// VMware signatures to clean from memory
static const CHAR* g_VMwareMemorySignatures[] = {
    "VMware",
    "VMWARE",
    "vmware",
    "VMX",
    "Hv#1",
    NULL
};

// Memory cleaning worker thread
static VOID MemoryCleanerThread(PVOID Context) {
    UNREFERENCED_PARAMETER(Context);
    
    VmLog("[MemClean] Memory cleaner thread started");
    
    LARGE_INTEGER interval;
    interval.QuadPart = -100000000LL; // 10 seconds
    
    while (g_MemoryCleanerActive) {
        // Check if stop event is signaled
        NTSTATUS status = KeWaitForSingleObject(
            &g_StopEvent,
            Executive,
            KernelMode,
            FALSE,
            &interval
        );
        
        if (status != STATUS_TIMEOUT) {
            // Stop event signaled
            break;
        }
        
        // Perform memory cleaning
        // Note: Scanning all kernel memory is dangerous and can cause BSOD
        // A production implementation would:
        // 1. Use MmGetPhysicalMemoryRanges to get memory ranges
        // 2. Use MmMapIoSpace to map physical pages
        // 3. Scan for VMware signatures
        // 4. Replace with innocuous strings
        // 5. This requires extreme caution to avoid crashes
        
        VmLog("[MemClean] Memory scan cycle (framework mode)");
    }
    
    VmLog("[MemClean] Memory cleaner thread exiting");
    PsTerminateSystemThread(STATUS_SUCCESS);
}

NTSTATUS MemoryCleanerInitialize(void) {
#if !ENABLE_MEMORY_CLEANING
    return STATUS_SUCCESS;
#endif
    
    VmLog("[MemClean] Initializing memory cleaner module...");
    
    KeInitializeEvent(&g_StopEvent, NotificationEvent, FALSE);
    
    // Create background thread for memory cleaning
    HANDLE threadHandle;
    OBJECT_ATTRIBUTES objAttr;
    InitializeObjectAttributes(&objAttr, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
    
    NTSTATUS status = PsCreateSystemThread(
        &threadHandle,
        THREAD_ALL_ACCESS,
        &objAttr,
        NULL,
        NULL,
        MemoryCleanerThread,
        NULL
    );
    
    if (!NT_SUCCESS(status)) {
        VmLog("[MemClean] Failed to create cleaner thread: 0x%08X", status);
        return status;
    }
    
    // Get thread object
    status = ObReferenceObjectByHandle(
        threadHandle,
        THREAD_ALL_ACCESS,
        *PsThreadType,
        KernelMode,
        &g_CleanerThread,
        NULL
    );
    
    ZwClose(threadHandle);
    
    if (!NT_SUCCESS(status)) {
        VmLog("[MemClean] Failed to reference thread object: 0x%08X", status);
        return status;
    }
    
    g_MemoryCleanerActive = TRUE;
    VmLog("[MemClean] Memory cleaner successfully initialized");
    VmLog("[MemClean] Note: Full memory scanning requires careful physical memory access");
    
    return STATUS_SUCCESS;
}

VOID MemoryCleanerCleanup(void) {
    if (g_MemoryCleanerActive) {
        VmLog("[MemClean] Cleaning up memory cleaner module...");
        
        // Signal thread to stop
        g_MemoryCleanerActive = FALSE;
        KeSetEvent(&g_StopEvent, IO_NO_INCREMENT, FALSE);
        
        // Wait for thread to exit
        if (g_CleanerThread) {
            KeWaitForSingleObject(
                g_CleanerThread,
                Executive,
                KernelMode,
                FALSE,
                NULL
            );
            
            ObDereferenceObject(g_CleanerThread);
            g_CleanerThread = NULL;
        }
        
        VmLog("[MemClean] Cleanup complete");
    }
}

BOOLEAN MemoryCleanerIsActive(void) {
    return g_MemoryCleanerActive;
}
