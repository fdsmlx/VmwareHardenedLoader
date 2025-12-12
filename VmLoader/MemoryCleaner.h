#pragma once
#ifndef VMLOADER_MEMORY_CLEANER_H
#define VMLOADER_MEMORY_CLEANER_H

#include <ntddk.h>

#ifdef __cplusplus
extern "C" {
#endif

// Memory Cleaner Module - Scans and cleans VMware artifacts from memory
// Removes VMware strings from kernel memory pools

NTSTATUS MemoryCleanerInitialize(void);
VOID MemoryCleanerCleanup(void);
BOOLEAN MemoryCleanerIsActive(void);

#ifdef __cplusplus
}
#endif

#endif // VMLOADER_MEMORY_CLEANER_H
