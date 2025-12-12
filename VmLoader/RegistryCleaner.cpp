#include <ntddk.h>
#include "RegistryCleaner.h"
#include "Config.h"
#include "Utils.h"

// Registry Cleaner Module - Intercepts registry queries to hide VMware artifacts
// Hooks registry APIs to modify or hide VMware-related values

static BOOLEAN g_RegistryCleanerActive = FALSE;

// Registry callback cookie
static LARGE_INTEGER g_RegistryCookie = {0};

// Registry strings to hide or replace
static const WCHAR* g_VMwareStrings[] = {
    L"VMware",
    L"VMWARE",
    L"vmware",
    L"Virtual",
    L"VIRTUAL",
    NULL
};

// Check if registry value contains VMware signatures
static BOOLEAN ContainsVMwareRegistry(PVOID data, ULONG dataSize) {
    if (!data || dataSize == 0) return FALSE;
    
    PWCHAR wideData = (PWCHAR)data;
    SIZE_T wideLen = dataSize / sizeof(WCHAR);
    
    for (SIZE_T i = 0; g_VMwareStrings[i] != NULL; i++) {
        SIZE_T sigLen = VmWideStringLength(g_VMwareStrings[i]);
        
        for (SIZE_T j = 0; j < wideLen - sigLen; j++) {
            if (RtlCompareMemory(&wideData[j], g_VMwareStrings[i], sigLen * sizeof(WCHAR)) == sigLen * sizeof(WCHAR)) {
                return TRUE;
            }
        }
    }
    
    return FALSE;
}

// Replace VMware strings in registry data
static VOID SpoofRegistryData(PVOID data, ULONG dataSize, ULONG type) {
    if (!data || dataSize == 0) return;
    
    if (type == REG_SZ || type == REG_EXPAND_SZ || type == REG_MULTI_SZ) {
        PWCHAR wideData = (PWCHAR)data;
        SIZE_T wideLen = dataSize / sizeof(WCHAR);
        
        // Replace "VMware" with "Dell  " (same length)
        for (SIZE_T i = 0; i < wideLen - 6; i++) {
            if (wideData[i] == L'V' && wideData[i+1] == L'M' && 
                wideData[i+2] == L'w' && wideData[i+3] == L'a' &&
                wideData[i+4] == L'r' && wideData[i+5] == L'e') {
                wideData[i] = L'D';
                wideData[i+1] = L'e';
                wideData[i+2] = L'l';
                wideData[i+3] = L'l';
                wideData[i+4] = L' ';
                wideData[i+5] = L' ';
                VmLog("[RegClean] Replaced VMware -> Dell in registry data");
            }
        }
        
        // Replace "Virtual" with "Desktop" (7 chars)
        for (SIZE_T i = 0; i < wideLen - 7; i++) {
            if (wideData[i] == L'V' && wideData[i+1] == L'i' && 
                wideData[i+2] == L'r' && wideData[i+3] == L't' &&
                wideData[i+4] == L'u' && wideData[i+5] == L'a' &&
                wideData[i+6] == L'l') {
                wideData[i] = L'D';
                wideData[i+1] = L'e';
                wideData[i+2] = L's';
                wideData[i+3] = L'k';
                wideData[i+4] = L't';
                wideData[i+5] = L'o';
                wideData[i+6] = L'p';
                VmLog("[RegClean] Replaced Virtual -> Desktop in registry data");
            }
        }
    }
}

// Registry callback notification routine
static NTSTATUS RegistryCallback(
    PVOID CallbackContext,
    PVOID Argument1,
    PVOID Argument2
) {
    UNREFERENCED_PARAMETER(CallbackContext);
    
    REG_NOTIFY_CLASS notifyClass = (REG_NOTIFY_CLASS)(ULONG_PTR)Argument1;
    
    // Handle post-query operations to modify returned data
    if (notifyClass == RegNtPostQueryValueKey) {
        PREG_POST_OPERATION_INFORMATION postInfo = (PREG_POST_OPERATION_INFORMATION)Argument2;
        
        if (NT_SUCCESS(postInfo->Status)) {
            PREG_QUERY_VALUE_KEY_INFORMATION queryInfo = 
                (PREG_QUERY_VALUE_KEY_INFORMATION)postInfo->PreInformation;
            
            if (queryInfo && queryInfo->Type) {
                ULONG type = *queryInfo->Type;
                
                if (queryInfo->Data && queryInfo->DataSize) {
                    __try {
                        if (ContainsVMwareRegistry(queryInfo->Data, *queryInfo->DataSize)) {
                            SpoofRegistryData(queryInfo->Data, *queryInfo->DataSize, type);
                        }
                    } __except (EXCEPTION_EXECUTE_HANDLER) {
                        // Ignore exceptions
                    }
                }
            }
        }
    }
    
    return STATUS_SUCCESS;
}

NTSTATUS RegistryCleanerInitialize(void) {
#if !ENABLE_REGISTRY_CLEANING
    return STATUS_SUCCESS;
#endif
    
    VmLog("[RegClean] Initializing registry cleaner module...");
    
    // Register registry callback
    UNICODE_STRING altitude;
    RtlInitUnicodeString(&altitude, L"385200"); // Altitude for registry callbacks
    
    NTSTATUS status = CmRegisterCallbackEx(
        RegistryCallback,
        &altitude,
        NULL, // Driver object
        NULL, // Context
        &g_RegistryCookie,
        NULL  // Reserved
    );
    
    if (!NT_SUCCESS(status)) {
        VmLog("[RegClean] Failed to register registry callback: 0x%08X", status);
        return status;
    }
    
    g_RegistryCleanerActive = TRUE;
    VmLog("[RegClean] Registry cleaner successfully initialized");
    
    return STATUS_SUCCESS;
}

VOID RegistryCleanerCleanup(void) {
    if (g_RegistryCleanerActive && g_RegistryCookie.QuadPart != 0) {
        VmLog("[RegClean] Cleaning up registry cleaner module...");
        
        CmUnRegisterCallback(g_RegistryCookie);
        g_RegistryCookie.QuadPart = 0;
        g_RegistryCleanerActive = FALSE;
        
        VmLog("[RegClean] Cleanup complete");
    }
}

BOOLEAN RegistryCleanerIsActive(void) {
    return g_RegistryCleanerActive;
}
