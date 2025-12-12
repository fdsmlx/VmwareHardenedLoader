#pragma once
#ifndef VMLOADER_WMI_INTERCEPTION_H
#define VMLOADER_WMI_INTERCEPTION_H

#include <ntddk.h>

#ifdef __cplusplus
extern "C" {
#endif

// WMI Interception Module - Intercepts WMI queries
// to hide VM-related information

NTSTATUS WmiInterceptionInitialize(void);
VOID WmiInterceptionCleanup(void);
BOOLEAN WmiInterceptionIsActive(void);

#ifdef __cplusplus
}
#endif

#endif // VMLOADER_WMI_INTERCEPTION_H
