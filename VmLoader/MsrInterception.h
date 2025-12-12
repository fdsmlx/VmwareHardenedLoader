#pragma once

// MSR Interception Module
// Intercepts Model Specific Register access to hide virtualization

#ifndef VMLOADER_MSR_INTERCEPTION_H
#define VMLOADER_MSR_INTERCEPTION_H

#include <ntddk.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize MSR interception functionality
NTSTATUS MsrInterceptionInitialize(void);

// Cleanup MSR interception functionality
VOID MsrInterceptionCleanup(void);

// Check if MSR interception is active
BOOLEAN MsrInterceptionIsActive(void);

#ifdef __cplusplus
}
#endif

#endif // VMLOADER_MSR_INTERCEPTION_H
