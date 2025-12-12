#pragma once

// CPUID Masking Module
// Hooks and spoofs CPUID instruction to hide hypervisor presence

#ifndef VMLOADER_CPUID_MASKING_H
#define VMLOADER_CPUID_MASKING_H

#include <ntddk.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize CPUID masking functionality
NTSTATUS CpuidMaskingInitialize(void);

// Cleanup CPUID masking functionality
VOID CpuidMaskingCleanup(void);

// Check if CPUID masking is active
BOOLEAN CpuidMaskingIsActive(void);

#ifdef __cplusplus
}
#endif

#endif // VMLOADER_CPUID_MASKING_H
