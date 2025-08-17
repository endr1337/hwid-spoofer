#ifndef SPOOF_H
#define SPOOF_H
#include <ntifs.h>
#include <wdm.h>
#include <ntstrsafe.h>
#include <acpiioct.h>
#include <ntdddisk.h>
#include <ntddk.h>
#include <ntddscsi.h>
#include <ntddstor.h>
void SpoofSerialNumber(char* serialNumber);

NTSTATUS StorageQueryCompletionRoutine(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
);

NTSTATUS SmartCompletionRoutine(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
);

NTSTATUS HookedMjDeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
);

NTSTATUS IrpHookDisk();

#endif /* SPOOF_H */
