#pragma once
#ifndef VMLOADER_PROCESS_HIDER_H
#define VMLOADER_PROCESS_HIDER_H
#include <ntddk.h>
#ifdef __cplusplus
extern "C" {
#endif
NTSTATUS ProcessHiderInitialize(void);
VOID ProcessHiderCleanup(void);
BOOLEAN ProcessHiderIsActive(void);
#ifdef __cplusplus
}
#endif
#endif
