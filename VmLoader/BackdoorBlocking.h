#pragma once

// VMware Backdoor Blocking Module
// Blocks VMware backdoor port communication

#ifndef VMLOADER_BACKDOOR_BLOCKING_H
#define VMLOADER_BACKDOOR_BLOCKING_H

#include <ntddk.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize backdoor blocking functionality
NTSTATUS BackdoorBlockingInitialize(void);

// Cleanup backdoor blocking functionality
VOID BackdoorBlockingCleanup(void);

// Check if backdoor blocking is active
BOOLEAN BackdoorBlockingIsActive(void);

#ifdef __cplusplus
}
#endif

#endif // VMLOADER_BACKDOOR_BLOCKING_H
