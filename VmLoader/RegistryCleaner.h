#pragma once
#ifndef VMLOADER_REGISTRY_CLEANER_H
#define VMLOADER_REGISTRY_CLEANER_H

#include <ntddk.h>

#ifdef __cplusplus
extern "C" {
#endif

// Registry Cleaner Module - Intercepts and modifies registry queries
// to hide VMware-related artifacts

NTSTATUS RegistryCleanerInitialize(void);
VOID RegistryCleanerCleanup(void);
BOOLEAN RegistryCleanerIsActive(void);

#ifdef __cplusplus
}
#endif

#endif // VMLOADER_REGISTRY_CLEANER_H
