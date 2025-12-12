#pragma once

// Timing Normalization Module
// Normalizes RDTSC/RDTSCP timing to mimic real hardware behavior

#ifndef VMLOADER_TIMING_NORMALIZATION_H
#define VMLOADER_TIMING_NORMALIZATION_H

#include <ntddk.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize timing normalization functionality
NTSTATUS TimingNormalizationInitialize(void);

// Cleanup timing normalization functionality
VOID TimingNormalizationCleanup(void);

// Check if timing normalization is active
BOOLEAN TimingNormalizationIsActive(void);

#ifdef __cplusplus
}
#endif

#endif // VMLOADER_TIMING_NORMALIZATION_H
