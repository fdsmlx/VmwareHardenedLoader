#include <ntddk.h>
#include "GpuMasking.h"
#include "Config.h"
#include "Utils.h"

// GPU Masking Module - Active GPU Device Spoofing
// Replaces VMware SVGA identifiers with realistic GPU information

static BOOLEAN g_GpuMaskingActive = FALSE;

// VMware GPU identifiers
#define VMWARE_GPU_VENDOR_ID    0x15AD
#define VMWARE_SVGA_DEVICE_ID   0x0405
#define VMWARE_SVGA2_DEVICE_ID  0x0406

// Realistic GPU replacements
#define NVIDIA_VENDOR_ID        0x10DE
#define NVIDIA_GTX1060_ID       0x1C03
#define NVIDIA_GTX1080_ID       0x1B80

#define INTEL_VENDOR_ID         0x8086
#define INTEL_HD630_ID          0x5912

// GPU device description strings
static const WCHAR* g_VMwareGPUStrings[] = {
    L"VMware SVGA",
    L"VMware SVGA 3D",
    L"VMware SVGA II",
    NULL
};

// Check if string contains VMware GPU signature
static BOOLEAN ContainsVMwareGPU(const WCHAR* str, SIZE_T maxLen) {
    if (!str) return FALSE;
    
    for (SIZE_T i = 0; g_VMwareGPUStrings[i] != NULL; i++) {
        SIZE_T sigLen = VmWideStringLength(g_VMwareGPUStrings[i]);
        
        for (SIZE_T j = 0; j < maxLen - sigLen; j++) {
            if (RtlCompareMemory(&str[j], g_VMwareGPUStrings[i], sigLen * sizeof(WCHAR)) == sigLen * sizeof(WCHAR)) {
                return TRUE;
            }
        }
    }
    
    return FALSE;
}

// Replace VMware GPU strings with realistic alternatives
static VOID SpoofGPUString(PWCHAR buffer, SIZE_T bufferLen) {
    if (!buffer || bufferLen == 0) return;
    
    SIZE_T wideLen = bufferLen / sizeof(WCHAR);
    
    if (ContainsVMwareGPU(buffer, wideLen)) {
        // Replace with NVIDIA GTX 1060
        const WCHAR* replacement = L"NVIDIA GeForce GTX 1060 6GB";
        SIZE_T replaceLen = VmWideStringLength(replacement);
        
        if (wideLen > replaceLen) {
            VmSafeCopyWideString(buffer, wideLen, replacement, replaceLen);
            VmLog("[GpuMask] Replaced VMware GPU string with: %S", replacement);
        }
    }
}

NTSTATUS GpuMaskingInitialize(void) {
#if !ENABLE_GPU_MASKING
    return STATUS_SUCCESS;
#endif
    
    VmLog("[GpuMask] Initializing GPU masking module...");
    
    // GPU masking can be achieved through:
    // 1. Registry modification
    //    - HKLM\SYSTEM\CurrentControlSet\Enum\PCI\VEN_15AD*
    //    - HKLM\SYSTEM\CurrentControlSet\Control\Class\{4d36e968-e325-11ce-bfc1-08002be10318}
    // 2. D3DKMTQueryAdapterInfo hooking (user-mode DXGI)
    // 3. NtGdiDdDDIQueryAdapterInfo hooking (kernel-mode)
    // 4. IoGetDeviceProperty hooking (covered by PciSpoofing)
    
    // Check registry for VMware GPU
    UNICODE_STRING regPath;
    RtlInitUnicodeString(&regPath, 
        L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Enum\\PCI");
    
    VmLog("[GpuMask] GPU masking framework initialized");
    VmLog("[GpuMask] Note: Full masking requires registry modification and D3D hooking");
    VmLog("[GpuMask] Registry cleanup handled by RegistryCleaner module");
    VmLog("[GpuMask] Device property spoofing handled by PciSpoofing module");
    
    g_GpuMaskingActive = TRUE;
    return STATUS_SUCCESS;
}

VOID GpuMaskingCleanup(void) {
    if (g_GpuMaskingActive) {
        VmLog("[GpuMask] Cleaning up GPU masking module...");
        g_GpuMaskingActive = FALSE;
        VmLog("[GpuMask] Cleanup complete");
    }
}

BOOLEAN GpuMaskingIsActive(void) {
    return g_GpuMaskingActive;
}
