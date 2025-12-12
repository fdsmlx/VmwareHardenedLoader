#pragma once

// VMware Hardened Loader - Global Configuration
// This header contains global configuration options for the anti-detection system

#ifndef VMLOADER_CONFIG_H
#define VMLOADER_CONFIG_H

// Feature Flags - Enable/Disable specific anti-detection modules
#define ENABLE_CPUID_MASKING        1  // CPUID instruction spoofing
#define ENABLE_TIMING_NORMALIZATION 1  // RDTSC/RDTSCP normalization
#define ENABLE_BACKDOOR_BLOCKING    1  // VMware backdoor port blocking
#define ENABLE_MSR_INTERCEPTION     1  // MSR read/write interception
#define ENABLE_PCI_SPOOFING         1  // PCI device ID spoofing
#define ENABLE_GPU_MASKING          1  // GPU device masking
#define ENABLE_DISK_SPOOFING        1  // Disk serial number spoofing
#define ENABLE_PROCESS_HIDING       1  // VMware process hiding
#define ENABLE_MAC_SPOOFING         1  // MAC address randomization
#define ENABLE_SMBIOS_SPOOFING      1  // Enhanced SMBIOS/ACPI spoofing

// Debug Options
#define VMLOADER_DEBUG_OUTPUT       1  // Enable debug prints
#define VMLOADER_TAG                'LMV' // Pool tag for memory allocations

// SMBIOS Spoofing Configuration
// Replace VMware identifiers with realistic hardware identifiers
#define SMBIOS_MANUFACTURER         "Dell Inc."
#define SMBIOS_PRODUCT_NAME         "OptiPlex 7050"
#define SMBIOS_VERSION              "1.0.8"
#define SMBIOS_SERIAL_PREFIX        "7G3MK"
#define SMBIOS_ASSET_TAG            "Asset-1234567890"

// CPUID Spoofing Configuration
// Mimic Intel Core i7-8700K processor characteristics
#define CPUID_VENDOR_STRING_EBX     0x756E6547  // "Genu"
#define CPUID_VENDOR_STRING_EDX     0x49656E69  // "ineI"
#define CPUID_VENDOR_STRING_ECX     0x6C65746E  // "ntel"
#define CPUID_BRAND_STRING          "Intel(R) Core(TM) i7-8700K CPU @ 3.70GHz"

// Timing Normalization Configuration
#define RDTSC_JITTER_PERCENT        5   // Percentage of jitter to add (+/-5%)
#define RDTSC_BASE_FREQUENCY        3700000000ULL  // 3.7 GHz in Hz

// PCI Device Spoofing Configuration
// Replace VMware device IDs with common real hardware
#define VMWARE_PCI_VENDOR_ID        0x15AD
#define SPOOF_PCI_VENDOR_ID         0x8086  // Intel Corporation
#define SPOOF_PCI_DEVICE_ID         0x1533  // Intel I210 Gigabit Network

// GPU Spoofing Configuration
#define VMWARE_GPU_DEVICE_ID        0x0405  // VMware SVGA II
#define SPOOF_GPU_VENDOR_ID         0x10DE  // NVIDIA Corporation
#define SPOOF_GPU_DEVICE_ID         0x1B80  // NVIDIA GeForce GTX 1080
#define SPOOF_GPU_NAME              "NVIDIA GeForce GTX 1080"

// Disk Spoofing Configuration
#define SPOOF_DISK_VENDOR           "Samsung"
#define SPOOF_DISK_MODEL            "SSD 860 EVO 500GB"
#define SPOOF_DISK_SERIAL_PREFIX    "S3Z"

// MAC Address Spoofing Configuration
// Use Intel OUI for spoofed MAC addresses
#define SPOOF_MAC_OUI_BYTE0         0x00
#define SPOOF_MAC_OUI_BYTE1         0x1B
#define SPOOF_MAC_OUI_BYTE2         0x21  // Intel Corporate

// VMware Process Names to Hide
static const WCHAR* g_ProcessesToHide[] = {
    L"vmtoolsd.exe",
    L"vmwaretray.exe",
    L"vmwareuser.exe",
    L"vm3dservice.exe",
    L"vmwareservice.exe",
    NULL  // Terminator
};

// VMware Backdoor Port Definitions
#define VMWARE_BACKDOOR_PORT        0x5658
#define VMWARE_BACKDOOR_HIGHBW_PORT 0x5659
#define VMWARE_BACKDOOR_MAGIC       0x564D5868  // "VMXh"

// MSR (Model Specific Register) Definitions
#define MSR_IA32_VMX_BASIC          0x480
#define MSR_IA32_VMX_PINBASED_CTLS  0x481
#define MSR_IA32_FEATURE_CONTROL    0x3A

// Windows Version Detection
#define WIN_VERSION_VISTA           6000
#define WIN_VERSION_WIN7            7600
#define WIN_VERSION_WIN8            9200
#define WIN_VERSION_WIN8_1          9600
#define WIN_VERSION_WIN10           10240
#define WIN_VERSION_WIN11           22000

// Seed for generating consistent random values across reboots
// This should be derived from hardware characteristics
#define SPOOFING_SEED_MULTIPLIER    0x9E3779B9  // Golden ratio prime

#endif // VMLOADER_CONFIG_H
