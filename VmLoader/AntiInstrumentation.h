#pragma once
#ifndef VMLOADER_ANTI_INSTRUMENTATION_H
#define VMLOADER_ANTI_INSTRUMENTATION_H

#include <ntddk.h>

#ifdef __cplusplus
extern "C" {
#endif

// Anti-Instrumentation Module - Protects against debugging and analysis
// Implements anti-debugging and anti-instrumentation techniques

NTSTATUS AntiInstrumentationInitialize(void);
VOID AntiInstrumentationCleanup(void);
BOOLEAN AntiInstrumentationIsActive(void);

#ifdef __cplusplus
}
#endif

#endif // VMLOADER_ANTI_INSTRUMENTATION_H
