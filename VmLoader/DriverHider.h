#pragma once
#ifndef VMLOADER_DRIVER_HIDER_H
#define VMLOADER_DRIVER_HIDER_H

#include <ntddk.h>

#ifdef __cplusplus
extern "C" {
#endif

// Driver Hider Module - Hides VMware drivers from enumeration
// Uses DKOM (Direct Kernel Object Manipulation) techniques

NTSTATUS DriverHiderInitialize(void);
VOID DriverHiderCleanup(void);
BOOLEAN DriverHiderIsActive(void);

#ifdef __cplusplus
}
#endif

#endif // VMLOADER_DRIVER_HIDER_H
