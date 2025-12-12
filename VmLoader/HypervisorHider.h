#pragma once
#ifndef VMLOADER_HYPERVISOR_HIDER_H
#define VMLOADER_HYPERVISOR_HIDER_H

#include <ntddk.h>

#ifdef __cplusplus
extern "C" {
#endif

// Hypervisor Hider Module - Removes hypervisor presence indicators
// Hides hypervisor-related strings and information

NTSTATUS HypervisorHiderInitialize(void);
VOID HypervisorHiderCleanup(void);
BOOLEAN HypervisorHiderIsActive(void);

#ifdef __cplusplus
}
#endif

#endif // VMLOADER_HYPERVISOR_HIDER_H
