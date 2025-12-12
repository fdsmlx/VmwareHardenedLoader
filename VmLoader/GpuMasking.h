#pragma once
#ifndef VMLOADER_GPU_MASKING_H
#define VMLOADER_GPU_MASKING_H
#include <ntddk.h>
#ifdef __cplusplus
extern "C" {
#endif
NTSTATUS GpuMaskingInitialize(void);
VOID GpuMaskingCleanup(void);
BOOLEAN GpuMaskingIsActive(void);
#ifdef __cplusplus
}
#endif
#endif
