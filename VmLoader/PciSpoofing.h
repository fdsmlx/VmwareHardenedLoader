#pragma once
#ifndef VMLOADER_PCI_SPOOFING_H
#define VMLOADER_PCI_SPOOFING_H
#include <ntddk.h>
#ifdef __cplusplus
extern "C" {
#endif
NTSTATUS PciSpoofingInitialize(void);
VOID PciSpoofingCleanup(void);
BOOLEAN PciSpoofingIsActive(void);
#ifdef __cplusplus
}
#endif
#endif
