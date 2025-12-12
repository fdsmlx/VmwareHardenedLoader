#pragma once
#ifndef VMLOADER_DISK_SPOOFING_H
#define VMLOADER_DISK_SPOOFING_H
#include <ntddk.h>
#ifdef __cplusplus
extern "C" {
#endif
NTSTATUS DiskSpoofingInitialize(void);
VOID DiskSpoofingCleanup(void);
BOOLEAN DiskSpoofingIsActive(void);
#ifdef __cplusplus
}
#endif
#endif
