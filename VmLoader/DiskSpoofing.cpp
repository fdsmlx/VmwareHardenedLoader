#include <ntifs.h>
#include <ntdddisk.h>
#include <ntddscsi.h>
#include "DiskSpoofing.h"
#include "Config.h"
#include "Utils.h"

// IoDriverObjectType is not guaranteed to be linkable; resolve it at runtime.
static POBJECT_TYPE *g_IoDriverObjectType = NULL;
extern "C" NTSTATUS ObReferenceObjectByName(
    PUNICODE_STRING ObjectName,
    ULONG Attributes,
    PACCESS_STATE AccessState,
    ACCESS_MASK DesiredAccess,
    POBJECT_TYPE ObjectType,
    KPROCESSOR_MODE AccessMode,
    PVOID ParseContext,
    PVOID *Object
);

// Disk Spoofing Module - Active IOCTL Interception
// This module intercepts disk-related IOCTLs and replaces VMware signatures
// with realistic hardware identifiers

static BOOLEAN g_DiskSpoofingActive = FALSE;
static PDRIVER_DISPATCH g_OriginalDiskDispatch = NULL;
static PDRIVER_OBJECT g_DiskDriverObject = NULL;

// Generate realistic disk serial number based on manufacturer
static VOID GenerateDiskSerial(PCHAR buffer, SIZE_T bufferSize, ULONG seed) {
    if (bufferSize < 20) return;
    
    // Samsung format: S3Z2NB0K123456A
    buffer[0] = 'S';
    buffer[1] = '3';
    buffer[2] = 'Z';
    buffer[3] = '2';
    buffer[4] = 'N';
    buffer[5] = 'B';
    buffer[6] = '0';
    buffer[7] = 'K';
    
    // Generate random hex digits
    for (SIZE_T i = 8; i < 14; i++) {
        ULONG random = VmGenerateSeededRandom(seed, (ULONG)i);
        ULONG digit = random % 36;
        if (digit < 10) {
            buffer[i] = '0' + (CHAR)digit;
        } else {
            buffer[i] = 'A' + (CHAR)(digit - 10);
        }
    }
    buffer[14] = 'A';
    buffer[15] = '\0';
}

// Check if string contains VMware signatures
static BOOLEAN ContainsVMwareSignature(const CHAR* str, SIZE_T maxLen) {
    if (!str) return FALSE;
    
    const CHAR* signatures[] = { "VMware", "VMWARE", "vmware", "Virtual", "VIRTUAL", "VBOX" };
    
    for (SIZE_T i = 0; i < sizeof(signatures) / sizeof(signatures[0]); i++) {
        SIZE_T sigLen = 0;
        while (signatures[i][sigLen]) sigLen++;
        
        if (maxLen < sigLen) continue;  // Skip if buffer too small
        
        for (SIZE_T j = 0; j <= maxLen - sigLen; j++) {
            if (RtlCompareMemory(&str[j], signatures[i], sigLen) == sigLen) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

// Replace VMware disk identifiers with spoofed values
static VOID SpoofStorageDescriptor(PSTORAGE_DEVICE_DESCRIPTOR descriptor, ULONG seed) {
    if (!descriptor || descriptor->Size < sizeof(STORAGE_DEVICE_DESCRIPTOR)) {
        return;
    }
    
    PCHAR baseAddr = (PCHAR)descriptor;
    ULONG maxOffset = descriptor->Size;
    
    // Spoof Vendor ID (e.g., "VMware" -> "Samsung")
    if (descriptor->VendorIdOffset > 0 && descriptor->VendorIdOffset < maxOffset) {
        PCHAR vendorId = baseAddr + descriptor->VendorIdOffset;
        SIZE_T remainingSize = maxOffset - descriptor->VendorIdOffset;
        
        if (ContainsVMwareSignature(vendorId, remainingSize)) {
            VmSafeCopyString(vendorId, remainingSize, "Samsung ", 8);
            VmLog("[DiskSpoof] Replaced vendor: Samsung");
        }
    }
    
    // Spoof Product ID (e.g., "Virtual disk" -> "SSD 860 EVO")
    if (descriptor->ProductIdOffset > 0 && descriptor->ProductIdOffset < maxOffset) {
        PCHAR productId = baseAddr + descriptor->ProductIdOffset;
        SIZE_T remainingSize = maxOffset - descriptor->ProductIdOffset;
        
        if (ContainsVMwareSignature(productId, remainingSize)) {
            VmSafeCopyString(productId, remainingSize, "SSD 860 EVO 500GB", 18);
            VmLog("[DiskSpoof] Replaced product: SSD 860 EVO 500GB");
        }
    }
    
    // Spoof Serial Number
    if (descriptor->SerialNumberOffset > 0 && descriptor->SerialNumberOffset < maxOffset) {
        PCHAR serialNumber = baseAddr + descriptor->SerialNumberOffset;
        SIZE_T remainingSize = maxOffset - descriptor->SerialNumberOffset;
        
        if (remainingSize >= 20) {
            GenerateDiskSerial(serialNumber, remainingSize, seed);
            VmLog("[DiskSpoof] Generated serial: %s", serialNumber);
        }
    }
    
    // Spoof Product Revision if it exists
    if (descriptor->ProductRevisionOffset > 0 && descriptor->ProductRevisionOffset < maxOffset) {
        PCHAR revision = baseAddr + descriptor->ProductRevisionOffset;
        SIZE_T remainingSize = maxOffset - descriptor->ProductRevisionOffset;
        
        if (remainingSize >= 8) {
            VmSafeCopyString(revision, remainingSize, "RVT01B6Q", 9);
        }
    }
}

// Hooked device control dispatch routine
static NTSTATUS HookedDiskDispatch(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp
) {
    if (!g_OriginalDiskDispatch) {
        return STATUS_NOT_SUPPORTED;
    }

    PIO_STACK_LOCATION ioStack = IoGetCurrentIrpStackLocation(Irp);
    BOOLEAN shouldSpoofDescriptor = FALSE;

    if (ioStack->MajorFunction == IRP_MJ_DEVICE_CONTROL &&
        ioStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_STORAGE_QUERY_PROPERTY &&
        ioStack->Parameters.DeviceIoControl.InputBufferLength >= sizeof(STORAGE_PROPERTY_QUERY)) {
        __try {
            PSTORAGE_PROPERTY_QUERY query =
                (PSTORAGE_PROPERTY_QUERY)Irp->AssociatedIrp.SystemBuffer;

            if (query &&
                query->PropertyId == StorageDeviceProperty &&
                query->QueryType == PropertyStandardQuery) {
                shouldSpoofDescriptor = TRUE;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            VmLog("[DiskSpoof] Exception reading IOCTL_STORAGE_QUERY_PROPERTY input");
        }
    }

    // This is a dispatch hook, not an attached filter. Calling IoCallDriver with
    // the same DeviceObject recursively re-enters this hook and can exhaust IRP
    // stack locations (NO_MORE_IRP_STACK_LOCATIONS).
    NTSTATUS status = g_OriginalDiskDispatch(DeviceObject, Irp);

    // We can safely patch METHOD_BUFFERED output only for synchronous completion.
    if (shouldSpoofDescriptor &&
        status != STATUS_PENDING &&
        NT_SUCCESS(status) &&
        ioStack->Parameters.DeviceIoControl.OutputBufferLength >= sizeof(STORAGE_DEVICE_DESCRIPTOR)) {
        __try {
            PSTORAGE_DEVICE_DESCRIPTOR descriptor =
                (PSTORAGE_DEVICE_DESCRIPTOR)Irp->AssociatedIrp.SystemBuffer;

            if (descriptor &&
                descriptor->Size >= sizeof(STORAGE_DEVICE_DESCRIPTOR) &&
                descriptor->Size <= ioStack->Parameters.DeviceIoControl.OutputBufferLength) {
                ULONG seed = VmGetHardwareSeed();
                SpoofStorageDescriptor(descriptor, seed);
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            VmLog("[DiskSpoof] Exception patching STORAGE_DEVICE_DESCRIPTOR");
        }
    }

    return status;
}

NTSTATUS DiskSpoofingInitialize(void) {
#if !ENABLE_DISK_SPOOFING
    return STATUS_SUCCESS;
#endif
    
    VmLog("[DiskSpoof] Initializing disk spoofing module...");
    
    // Get disk driver object
    UNICODE_STRING diskDriverName;
    RtlInitUnicodeString(&diskDriverName, L"\\Driver\\Disk");
    
    if (!g_IoDriverObjectType) {
        UNICODE_STRING routineName;
        RtlInitUnicodeString(&routineName, L"IoDriverObjectType");
        g_IoDriverObjectType = (POBJECT_TYPE*)MmGetSystemRoutineAddress(&routineName);
        if (!g_IoDriverObjectType) {
            VmLog("[DiskSpoof] IoDriverObjectType not found");
            return STATUS_PROCEDURE_NOT_FOUND;
        }
    }

    NTSTATUS status = ObReferenceObjectByName(
        &diskDriverName,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        NULL,
        0,
        *g_IoDriverObjectType,
        KernelMode,
        NULL,
        (PVOID*)&g_DiskDriverObject
    );
    
    if (!NT_SUCCESS(status)) {
        VmLog("[DiskSpoof] Failed to get disk driver object: 0x%08X", status);
        return status;
    }
    
    // Hook IRP_MJ_DEVICE_CONTROL
    g_OriginalDiskDispatch = g_DiskDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL];
    InterlockedExchangePointer(
        (PVOID*)&g_DiskDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL],
        (PVOID)HookedDiskDispatch
    );
    
    g_DiskSpoofingActive = TRUE;
    VmLog("[DiskSpoof] Successfully hooked disk driver (IRP_MJ_DEVICE_CONTROL)");
    
    return STATUS_SUCCESS;
}

VOID DiskSpoofingCleanup(void) {
    if (g_DiskSpoofingActive && g_DiskDriverObject && g_OriginalDiskDispatch) {
        VmLog("[DiskSpoof] Cleaning up disk spoofing module...");
        
        // Restore original dispatch routine
        InterlockedExchangePointer(
            (PVOID*)&g_DiskDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL],
            (PVOID)g_OriginalDiskDispatch
        );
        
        ObDereferenceObject(g_DiskDriverObject);
        g_DiskDriverObject = NULL;
        g_OriginalDiskDispatch = NULL;
        g_DiskSpoofingActive = FALSE;
        
        VmLog("[DiskSpoof] Cleanup complete");
    }
}

BOOLEAN DiskSpoofingIsActive(void) {
    return g_DiskSpoofingActive;
}
