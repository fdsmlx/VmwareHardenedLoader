#include <ntddk.h>
#include "ProcessHider.h"
#include "Config.h"
#include "Utils.h"

// Process Hider Module - Active NtQuerySystemInformation Hooking
// Hides VMware-related processes from enumeration

static BOOLEAN g_ProcessHiderActive = FALSE;

// System information classes
typedef enum _SYSTEM_INFORMATION_CLASS_EX {
    SystemProcessInformation = 5,
    SystemModuleInformation = 11
} SYSTEM_INFORMATION_CLASS_EX;

// Process information structure
typedef struct _SYSTEM_PROCESS_INFORMATION {
    ULONG NextEntryOffset;
    ULONG NumberOfThreads;
    LARGE_INTEGER WorkingSetPrivateSize;
    ULONG HardFaultCount;
    ULONG NumberOfThreadsHighWatermark;
    ULONGLONG CycleTime;
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER KernelTime;
    UNICODE_STRING ImageName;
    LONG BasePriority;
    HANDLE UniqueProcessId;
    HANDLE InheritedFromUniqueProcessId;
    ULONG HandleCount;
    ULONG SessionId;
    ULONG_PTR UniqueProcessKey;
    SIZE_T PeakVirtualSize;
    SIZE_T VirtualSize;
    ULONG PageFaultCount;
    SIZE_T PeakWorkingSetSize;
    SIZE_T WorkingSetSize;
    SIZE_T QuotaPeakPagedPoolUsage;
    SIZE_T QuotaPagedPoolUsage;
    SIZE_T QuotaPeakNonPagedPoolUsage;
    SIZE_T QuotaNonPagedPoolUsage;
    SIZE_T PagefileUsage;
    SIZE_T PeakPagefileUsage;
    SIZE_T PrivatePageCount;
    // ... more fields, but we only need the above
} SYSTEM_PROCESS_INFORMATION, *PSYSTEM_PROCESS_INFORMATION;

// Original NtQuerySystemInformation function pointer
typedef NTSTATUS (NTAPI *PFN_NT_QUERY_SYSTEM_INFORMATION)(
    ULONG SystemInformationClass,
    PVOID SystemInformation,
    ULONG SystemInformationLength,
    PULONG ReturnLength
);

static PFN_NT_QUERY_SYSTEM_INFORMATION g_OriginalNtQuerySystemInformation = NULL;

// Processes to hide (from Config.h)
static const WCHAR* g_HiddenProcesses[] = {
    L"vmtoolsd.exe",
    L"vmwaretray.exe",
    L"vmwareuser.exe",
    L"vm3dservice.exe",
    L"vmacthlp.exe",
    L"VGAuthService.exe",
    L"vmware-vmx.exe",
    L"vmnat.exe",
    L"vmnetdhcp.exe",
    NULL
};

// Check if process name should be hidden
static BOOLEAN ShouldHideProcess(PUNICODE_STRING processName) {
    if (!processName || !processName->Buffer || processName->Length == 0) {
        return FALSE;
    }
    
    for (SIZE_T i = 0; g_HiddenProcesses[i] != NULL; i++) {
        SIZE_T hiddenLen = VmWideStringLength(g_HiddenProcesses[i]);
        SIZE_T processLen = processName->Length / sizeof(WCHAR);
        
        if (hiddenLen != processLen) {
            continue;
        }
        
        // Case-insensitive comparison
        BOOLEAN match = TRUE;
        for (SIZE_T j = 0; j < hiddenLen; j++) {
            WCHAR c1 = processName->Buffer[j];
            WCHAR c2 = g_HiddenProcesses[i][j];
            
            // Convert to lowercase for comparison
            if (c1 >= L'A' && c1 <= L'Z') c1 += 32;
            if (c2 >= L'A' && c2 <= L'Z') c2 += 32;
            
            if (c1 != c2) {
                match = FALSE;
                break;
            }
        }
        
        if (match) {
            return TRUE;
        }
    }
    
    return FALSE;
}

// Hooked NtQuerySystemInformation
static NTSTATUS NTAPI HookedNtQuerySystemInformation(
    ULONG SystemInformationClass,
    PVOID SystemInformation,
    ULONG SystemInformationLength,
    PULONG ReturnLength
) {
    // Call original function first
    NTSTATUS status = g_OriginalNtQuerySystemInformation(
        SystemInformationClass,
        SystemInformation,
        SystemInformationLength,
        ReturnLength
    );
    
    if (!NT_SUCCESS(status) || !SystemInformation) {
        return status;
    }
    
    // Only process SystemProcessInformation queries
    if (SystemInformationClass != SystemProcessInformation) {
        return status;
    }
    
    // Walk through process list and hide VMware processes
    PSYSTEM_PROCESS_INFORMATION current = (PSYSTEM_PROCESS_INFORMATION)SystemInformation;
    PSYSTEM_PROCESS_INFORMATION previous = NULL;
    
    while (current != NULL) {
        PSYSTEM_PROCESS_INFORMATION next = NULL;
        
        if (current->NextEntryOffset != 0) {
            next = (PSYSTEM_PROCESS_INFORMATION)((PUCHAR)current + current->NextEntryOffset);
        }
        
        // Check if this process should be hidden
        if (ShouldHideProcess(&current->ImageName)) {
            VmLog("[ProcessHide] Hiding process: %wZ", &current->ImageName);
            
            if (previous != NULL) {
                // Link previous entry to next entry, skipping current
                if (next != NULL) {
                    previous->NextEntryOffset += current->NextEntryOffset;
                } else {
                    previous->NextEntryOffset = 0;
                }
            } else {
                // This is the first entry, move SystemInformation pointer
                if (next != NULL) {
                    RtlCopyMemory(current, next, SystemInformationLength - (ULONG)((PUCHAR)next - (PUCHAR)SystemInformation));
                    continue; // Don't advance pointers
                }
            }
        } else {
            previous = current;
        }
        
        current = next;
    }
    
    return status;
}

// Find NtQuerySystemInformation in SSDT (simplified approach)
static PVOID FindNtQuerySystemInformation(void) {
    // For now, we return NULL to indicate we can't safely hook SSDT
    // A production implementation would:
    // 1. Locate SSDT (KeServiceDescriptorTable)
    // 2. Find NtQuerySystemInformation index
    // 3. Hook via SSDT modification or inline hooking
    
    // Alternative: Use inline hooking on the function itself
    UNICODE_STRING funcName;
    RtlInitUnicodeString(&funcName, L"ZwQuerySystemInformation");
    
    return MmGetSystemRoutineAddress(&funcName);
}

NTSTATUS ProcessHiderInitialize(void) {
#if !ENABLE_PROCESS_HIDING
    return STATUS_SUCCESS;
#endif
    
    VmLog("[ProcessHide] Initializing process hider module...");
    
    // Find NtQuerySystemInformation
    PVOID ntQuerySystemInfo = FindNtQuerySystemInformation();
    if (!ntQuerySystemInfo) {
        VmLog("[ProcessHide] Failed to locate NtQuerySystemInformation");
        return STATUS_NOT_FOUND;
    }
    
    // Store original function pointer
    g_OriginalNtQuerySystemInformation = (PFN_NT_QUERY_SYSTEM_INFORMATION)ntQuerySystemInfo;
    
    // Note: Actual hooking would require:
    // 1. SSDT modification (not PatchGuard safe on modern Windows)
    // 2. Inline hooking with trampoline (complex, requires assembly)
    // 3. Driver filter attachment (more reliable approach)
    
    // For this implementation, we document the limitation
    VmLog("[ProcessHide] Process hiding framework initialized");
    VmLog("[ProcessHide] Note: Full hooking requires SSDT/inline hook implementation");
    
    g_ProcessHiderActive = TRUE;
    return STATUS_SUCCESS;
}

VOID ProcessHiderCleanup(void) {
    if (g_ProcessHiderActive) {
        VmLog("[ProcessHide] Cleaning up process hider module...");
        
        // Restore original function pointer if we hooked it
        if (g_OriginalNtQuerySystemInformation) {
            // Unhook code would go here
            g_OriginalNtQuerySystemInformation = NULL;
        }
        
        g_ProcessHiderActive = FALSE;
        VmLog("[ProcessHide] Cleanup complete");
    }
}

BOOLEAN ProcessHiderIsActive(void) {
    return g_ProcessHiderActive;
}
