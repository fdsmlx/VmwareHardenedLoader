#include <ntddk.h>
#include <ntdddisk.h>
#include <ntddscsi.h>
#include "DiskSpoofing.h"
#include "Config.h"
#include "Utils.h"

// External kernel variables
extern POBJECT_TYPE IoDriverObjectType;

// Disk Spoofing Module - Active IOCTL Interception
// This module intercepts disk-related IOCTLs and replaces VMware signatures
// with realistic hardware identifiers

static BOOLEAN g_DiskSpoofingActive = FALSE;
static PDRIVER_DISPATCH g_OriginalDiskDispatch = NULL;
static PDRIVER_OBJECT g_DiskDriverObject = NULL;

// Storage property query structures
typedef enum _STORAGE_PROPERTY_ID {
    StorageDeviceProperty = 0,
    StorageAdapterProperty = 1,
    StorageDeviceIdProperty = 2,
    StorageDeviceUniqueIdProperty = 3,
    StorageDeviceWriteCacheProperty = 4,
    StorageMiniportProperty = 5,
    StorageAccessAlignmentProperty = 6,
    StorageDeviceSeekPenaltyProperty = 7,
    StorageDeviceTrimProperty = 8
} STORAGE_PROPERTY_ID, *PSTORAGE_PROPERTY_ID;

typedef enum _STORAGE_QUERY_TYPE {
    PropertyStandardQuery = 0,
    PropertyExistsQuery = 1,
    PropertyMaskQuery = 2,
    PropertyQueryMaxDefined = 3
} STORAGE_QUERY_TYPE, *PSTORAGE_QUERY_TYPE;

typedef struct _STORAGE_PROPERTY_QUERY {
    STORAGE_PROPERTY_ID PropertyId;
    STORAGE_QUERY_TYPE QueryType;
    UCHAR AdditionalParameters[1];
} STORAGE_PROPERTY_QUERY, *PSTORAGE_PROPERTY_QUERY;

typedef struct _STORAGE_DEVICE_DESCRIPTOR {
    ULONG Version;
    ULONG Size;
    UCHAR DeviceType;
    UCHAR DeviceTypeModifier;
    BOOLEAN RemovableMedia;
    BOOLEAN CommandQueueing;
    ULONG VendorIdOffset;
    ULONG ProductIdOffset;
    ULONG ProductRevisionOffset;
    ULONG SerialNumberOffset;
    ULONG BusType;
    ULONG RawPropertiesLength;
    UCHAR RawDeviceProperties[1];
} STORAGE_DEVICE_DESCRIPTOR, *PSTORAGE_DEVICE_DESCRIPTOR;

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

// IRP completion routine for storage queries
static NTSTATUS DiskSpoofingCompletionRoutine(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_reads_opt_(_Inexpressible_("varies")) PVOID Context
) {
    UNREFERENCED_PARAMETER(DeviceObject);
    
    if (Irp->PendingReturned) {
        IoMarkIrpPending(Irp);
    }
    
    PIO_STACK_LOCATION ioStack = IoGetCurrentIrpStackLocation(Irp);
    
    // Check if this is a successful IOCTL_STORAGE_QUERY_PROPERTY
    if (NT_SUCCESS(Irp->IoStatus.Status) &&
        ioStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_STORAGE_QUERY_PROPERTY) {
        
        PSTORAGE_PROPERTY_QUERY query = (PSTORAGE_PROPERTY_QUERY)Irp->AssociatedIrp.SystemBuffer;
        
        if (query && query->PropertyId == StorageDeviceProperty && 
            query->QueryType == PropertyStandardQuery) {
            
            PSTORAGE_DEVICE_DESCRIPTOR descriptor = (PSTORAGE_DEVICE_DESCRIPTOR)Irp->AssociatedIrp.SystemBuffer;
            
            if (descriptor && descriptor->Size >= sizeof(STORAGE_DEVICE_DESCRIPTOR)) {
                ULONG seed = VmGetHardwareSeed();
                SpoofStorageDescriptor(descriptor, seed);
            }
        }
    }
    
    if (Context) {
        KeSetEvent((PKEVENT)Context, IO_NO_INCREMENT, FALSE);
    }
    
    return STATUS_SUCCESS;
}

// Hooked device control dispatch routine
static NTSTATUS HookedDiskDispatch(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp
) {
    PIO_STACK_LOCATION ioStack = IoGetCurrentIrpStackLocation(Irp);
    
    // Only intercept IOCTL_STORAGE_QUERY_PROPERTY
    if (ioStack->MajorFunction == IRP_MJ_DEVICE_CONTROL) {
        ULONG ioControlCode = ioStack->Parameters.DeviceIoControl.IoControlCode;
        
        if (ioControlCode == IOCTL_STORAGE_QUERY_PROPERTY) {
            // Set up completion routine to modify the response
            KEVENT event;
            KeInitializeEvent(&event, NotificationEvent, FALSE);
            
            IoCopyCurrentIrpStackLocationToNext(Irp);
            IoSetCompletionRoutine(Irp, DiskSpoofingCompletionRoutine, 
                                   &event, TRUE, TRUE, TRUE);
            
            NTSTATUS status = IoCallDriver(DeviceObject, Irp);
            
            if (status == STATUS_PENDING) {
                KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
                status = Irp->IoStatus.Status;
            }
            
            return status;
        }
    }
    
    // Pass through to original handler
    if (g_OriginalDiskDispatch) {
        return g_OriginalDiskDispatch(DeviceObject, Irp);
    }
    
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS DiskSpoofingInitialize(void) {
#if !ENABLE_DISK_SPOOFING
    return STATUS_SUCCESS;
#endif
    
    VmLog("[DiskSpoof] Initializing disk spoofing module...");
    
    // Get disk driver object
    UNICODE_STRING diskDriverName;
    RtlInitUnicodeString(&diskDriverName, L"\\Driver\\Disk");
    
    NTSTATUS status = ObReferenceObjectByName(
        &diskDriverName,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        NULL,
        0,
        *IoDriverObjectType,
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
