#pragma once
#ifndef VMLOADER_MAC_SPOOFING_H
#define VMLOADER_MAC_SPOOFING_H
#include <ntddk.h>
#ifdef __cplusplus
extern "C" {
#endif
NTSTATUS MacSpoofingInitialize(void);
VOID MacSpoofingCleanup(void);
BOOLEAN MacSpoofingIsActive(void);
#ifdef __cplusplus
}
#endif
#endif
