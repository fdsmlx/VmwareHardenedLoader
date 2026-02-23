#pragma once

// VMware Hardened Loader - Utility Functions
// Common utility functions used across multiple modules

#ifndef VMLOADER_UTILS_H
#define VMLOADER_UTILS_H

#include <ntddk.h>
#include <intrin.h>
#include "Config.h"

#ifdef __cplusplus
extern "C" {
#endif

// Debug printing macro
#if VMLOADER_DEBUG_OUTPUT
#define VmLog(format, ...) DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, "[VmLoader] " format "\n", ##__VA_ARGS__)
#else
#define VmLog(format, ...) ((void)0)
#endif

// Memory allocation helpers
__forceinline PVOID VmAllocatePool(SIZE_T size) {
    return ExAllocatePoolWithTag(NonPagedPool, size, VMLOADER_TAG);
}

__forceinline VOID VmFreePool(PVOID ptr) {
    if (ptr) {
        ExFreePoolWithTag(ptr, VMLOADER_TAG);
    }
}

// String utilities
__forceinline BOOLEAN VmIsStringMatch(const char* str1, const char* str2, SIZE_T length) {
    return RtlCompareMemory(str1, str2, length) == length;
}

// Helper for wide string length in kernel mode
__forceinline SIZE_T VmWideStringLength(const WCHAR* str) {
    if (!str) return 0;
    SIZE_T len = 0;
    while (str[len] != L'\0') {
        len++;
    }
    return len;
}

__forceinline BOOLEAN VmIsWideStringMatch(const WCHAR* str1, const WCHAR* str2) {
    if (!str1 || !str2) return FALSE;
    SIZE_T len1 = VmWideStringLength(str1);
    SIZE_T len2 = VmWideStringLength(str2);
    if (len1 != len2) return FALSE;
    return RtlCompareMemory(str1, str2, len1 * sizeof(WCHAR)) == (len1 * sizeof(WCHAR));
}

// Search for pattern in memory
_Use_decl_annotations_ 
static inline PVOID VmMemMem(const void* search_base, SIZE_T search_size, 
                              const void* pattern, SIZE_T pattern_size) {
    if (pattern_size > search_size) {
        return NULL;
    }
    const char* base = (const char*)search_base;
    for (SIZE_T i = 0; i <= search_size - pattern_size; i++) {
        if (RtlCompareMemory(pattern, &base[i], pattern_size) == pattern_size) {
            return (PVOID)&base[i];
        }
    }
    return NULL;
}

// Windows version detection
typedef struct _VM_OS_VERSION {
    ULONG BuildNumber;
    BOOLEAN IsWindows11;
    BOOLEAN IsWindows10;
    BOOLEAN IsWindows8;
    BOOLEAN IsWindows7;
    BOOLEAN IsVista;
} VM_OS_VERSION, *PVM_OS_VERSION;

// Get current Windows version
_Use_decl_annotations_
static inline NTSTATUS VmGetOsVersion(PVM_OS_VERSION version) {
    RTL_OSVERSIONINFOW osVersion = { 0 };
    osVersion.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOW);
    
    NTSTATUS status = RtlGetVersion(&osVersion);
    if (!NT_SUCCESS(status)) {
        return status;
    }
    
    RtlZeroMemory(version, sizeof(VM_OS_VERSION));
    version->BuildNumber = osVersion.dwBuildNumber;
    
    // Determine OS version
    if (osVersion.dwMajorVersion == 6) {
        if (osVersion.dwMinorVersion == 0) {
            version->IsVista = TRUE;
        } else if (osVersion.dwMinorVersion == 1) {
            version->IsWindows7 = TRUE;
        } else if (osVersion.dwMinorVersion == 2) {
            version->IsWindows8 = TRUE;
        } else if (osVersion.dwMinorVersion == 3) {
            version->IsWindows8 = TRUE; // Windows 8.1
        }
    } else if (osVersion.dwMajorVersion == 10) {
        if (osVersion.dwBuildNumber >= WIN_VERSION_WIN11) {
            version->IsWindows11 = TRUE;
        } else {
            version->IsWindows10 = TRUE;
        }
    }
    
    return STATUS_SUCCESS;
}

// Generate consistent pseudo-random number based on seed
_Use_decl_annotations_
static inline ULONG VmGenerateSeededRandom(ULONG seed, ULONG index) {
    // Simple LCG (Linear Congruential Generator)
    ULONG value = seed;
    for (ULONG i = 0; i <= index; i++) {
        value = (value * 1103515245 + 12345) & 0x7FFFFFFF;
    }
    return value;
}

// Generate hardware-based seed for consistent spoofing
_Use_decl_annotations_
static inline ULONG VmGetHardwareSeed(void) {
    ULONG seed = 0;
    
    // Use CPU timestamp as base
    LARGE_INTEGER tickCount;
    KeQueryTickCount(&tickCount);
    seed ^= (ULONG)(tickCount.QuadPart & 0xFFFFFFFF);
    
    // Mix in some system information
    seed ^= KeQueryActiveProcessorCount(NULL);
    seed ^= (ULONG)(ULONG_PTR)PsGetCurrentProcess();
    
    // Apply multiplier to distribute bits
    seed *= SPOOFING_SEED_MULTIPLIER;
    
    return seed;
}

// Generate spoofed serial number
_Use_decl_annotations_
static inline VOID VmGenerateSerialNumber(char* buffer, SIZE_T bufferSize, 
                                          const char* prefix, ULONG seed) {
    if (bufferSize < 16) {
        return;
    }
    
    // Calculate prefix length manually (kernel mode safe)
    SIZE_T prefixLen = 0;
    while (prefix[prefixLen] != '\0' && prefixLen < bufferSize - 8) {
        prefixLen++;
    }
    
    if (prefixLen > bufferSize - 8) {
        prefixLen = bufferSize - 8;
    }
    
    // Copy prefix
    RtlCopyMemory(buffer, prefix, prefixLen);
    
    // Generate random suffix
    for (SIZE_T i = prefixLen; i < bufferSize - 1; i++) {
        ULONG random = VmGenerateSeededRandom(seed, (ULONG)i);
        buffer[i] = 'A' + (char)(random % 26);
    }
    buffer[bufferSize - 1] = '\0';
}

// Check if running inside VMware
_Use_decl_annotations_
static inline BOOLEAN VmIsRunningInVmware(void) {
    // Simple check via CPUID
    int cpuInfo[4] = { 0 };
    __cpuid(cpuInfo, 0x40000000);
    
    // Check for "VMwareVMware" signature
    if (cpuInfo[1] == 0x61774D56 && // "VMwa"
        cpuInfo[2] == 0x4D566572 && // "reVM"
        cpuInfo[3] == 0x65726177) { // "ware"
        return TRUE;
    }
    
    return FALSE;
}

// Safe string copy
_Use_decl_annotations_
static inline NTSTATUS VmSafeCopyString(char* dest, SIZE_T destSize, 
                                        const char* src, SIZE_T srcSize) {
    if (!dest || !src || destSize == 0) {
        return STATUS_INVALID_PARAMETER;
    }
    
    SIZE_T copySize = ((destSize - 1) < srcSize) ? (destSize - 1) : srcSize;
    RtlCopyMemory(dest, src, copySize);
    dest[copySize] = '\0';
    
    return STATUS_SUCCESS;
}

// Safe wide string copy
_Use_decl_annotations_
static inline NTSTATUS VmSafeCopyWideString(WCHAR* dest, SIZE_T destSize,
                                             const WCHAR* src, SIZE_T srcSize) {
    if (!dest || !src || destSize == 0) {
        return STATUS_INVALID_PARAMETER;
    }
    
    SIZE_T copySize = ((destSize - 1) < srcSize) ? (destSize - 1) : srcSize;
    RtlCopyMemory(dest, src, copySize * sizeof(WCHAR));
    dest[copySize] = L'\0';
    
    return STATUS_SUCCESS;
}

// Module initialization status tracking
typedef struct _VM_MODULE_STATUS {
    BOOLEAN CpuidMaskingEnabled;
    BOOLEAN TimingNormalizationEnabled;
    BOOLEAN BackdoorBlockingEnabled;
    BOOLEAN MsrInterceptionEnabled;
    BOOLEAN PciSpoofingEnabled;
    BOOLEAN GpuMaskingEnabled;
    BOOLEAN DiskSpoofingEnabled;
    BOOLEAN ProcessHidingEnabled;
    BOOLEAN MacSpoofingEnabled;
    BOOLEAN SmbiosSpoofingEnabled;
    BOOLEAN RegistryCleaningEnabled;
    BOOLEAN DriverHidingEnabled;
    BOOLEAN WmiInterceptionEnabled;
    BOOLEAN MemoryCleaningEnabled;
    BOOLEAN HypervisorHidingEnabled;
    BOOLEAN AntiInstrumentationEnabled;
} VM_MODULE_STATUS, *PVM_MODULE_STATUS;

// Global module status (defined in main.cpp)
extern VM_MODULE_STATUS g_ModuleStatus;

// Module initialization function type
typedef NTSTATUS (*PFN_MODULE_INIT)(void);
typedef VOID (*PFN_MODULE_CLEANUP)(void);

#ifdef __cplusplus
}
#endif

#endif // VMLOADER_UTILS_H
