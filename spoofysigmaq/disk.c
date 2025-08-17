#include <ntifs.h>
#include <ntstrsafe.h>
#include <acpiioct.h>
#include <ntdddisk.h>
#include <ntddk.h>
#include <ntddscsi.h>
#include <ntddstor.h>
#include "spoof.h"
#include "shared.h"
#pragma warning(disable : 4996)
/*

*/

#define IOCTL_STORAGE_QUERY_PROPERTY CTL_CODE(IOCTL_STORAGE_BASE, 0x0500, METHOD_BUFFERED, FILE_ANY_ACCESS)

extern POBJECT_TYPE* IoDriverObjectType;
PDRIVER_DISPATCH OldIrpMj;
char NumTable[] = "1234567890";
char SpoofedHWID[] = "XXYXXYXXXXYXY"; 
char NonNullHWID[] = "XXYXXYXXXXYXY";
char nulledid[] = "                           ";
int HWIDGenerated = 0; 
typedef struct _REQUEST_STRUCT
{
	PIO_COMPLETION_ROUTINE OldRoutine;
	PVOID OldContext;
	ULONG OutputBufferLength;
	PSTORAGE_DEVICE_DESCRIPTOR StorageDescriptor;
} REQUEST_STRUCT;

void NullSerialNumber(char* serialNumber) {
	if (strlen(NonNullHWID) >= 25) {
		memset(NonNullHWID, 0, sizeof(NonNullHWID));
		memcpy(serialNumber, NonNullHWID, 21); 
	}
	else {
	}
}
VOID SpoofSerialNumber(char* serialNumber)
{
	NullSerialNumber(serialNumber);



	if (HWIDGenerated == 0)
	{
		HWIDGenerated = 1;
		LARGE_INTEGER Seed;
		KeQuerySystemTimePrecise(&Seed);

		for (int i = 0; i < strlen(NonNullHWID); ++i)
		{
			if (NonNullHWID[i] == 'Y')
			{
				NonNullHWID[i] = RtlRandomEx(&Seed.LowPart) % 26 + 65;
			}

			if (NonNullHWID[i] == 'X')
			{
				NonNullHWID[i] = NumTable[RtlRandomEx(&Seed.LowPart) % (strlen(NumTable) - 1)];
			}
		}
	}
	//DbgPrint("SpoofedHWID: %s\n", SpoofedHWID);
	memcpy((void*)serialNumber,
		(void*)NonNullHWID,
		21);

}



NTSTATUS SmartCompletionRoutine(
	PDEVICE_OBJECT DeviceObject,
	PIRP Irp,
	PVOID Context
)
{
	UNREFERENCED_PARAMETER(DeviceObject);

	//Grab old routine and free buffer
	PIO_COMPLETION_ROUTINE OldCompletionRoutine = NULL;
	PVOID OldContext = NULL;

	if (Context != NULL)
	{
		REQUEST_STRUCT* pRequest = (REQUEST_STRUCT*)Context;
		OldCompletionRoutine = pRequest->OldRoutine;
		OldContext = pRequest->OldContext;
		ExFreePool(Context);
	}

	//DbgPrint("%s: Returning STATUS_NOT_SUPPORTED\n", __FUNCTION__);

	Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

	// Call next completion routine (if any)
	//if ((Irp->StackCount > (ULONG)1) && (OldCompletionRoutine != NULL))
	//	return OldCompletionRoutine(DeviceObject, Irp, OldContext);
	                                                                                                                                                               
	return Irp->IoStatus.Status;
}
NTSTATUS StorageQueryCompletionRoutine(
	PDEVICE_OBJECT DeviceObject,
	PIRP Irp,
	PVOID Context
)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	UNREFERENCED_PARAMETER(Context);

	//Grab old routine and free buffer
	PIO_COMPLETION_ROUTINE OldCompletionRoutine = NULL;
	PVOID OldContext = NULL;
	ULONG OutputBufferLength = 0;
	PSTORAGE_DEVICE_DESCRIPTOR descriptor = NULL;

	if (Context != NULL)
	{
		REQUEST_STRUCT* pRequest = (REQUEST_STRUCT*)Context;
		OldCompletionRoutine = pRequest->OldRoutine;
		OldContext = pRequest->OldContext;
		OutputBufferLength = pRequest->OutputBufferLength;
		descriptor = pRequest->StorageDescriptor;

		ExFreePool(Context);
	}

	//DbgPrint("SerialNumberOffset: %i, OutputBufferLength: %i\n", descriptor->SerialNumberOffset, OutputBufferLength);

	if (FIELD_OFFSET(STORAGE_DEVICE_DESCRIPTOR, SerialNumberOffset) < (LONG)OutputBufferLength &&
		descriptor->SerialNumberOffset > 0 &&
		descriptor->SerialNumberOffset < OutputBufferLength)
	{
		char* SerialNumber = ((char*)descriptor) + descriptor->SerialNumberOffset;
		//DbgPrint("%s: SerialNumber: %s\n", __FUNCTION__,
		//	SerialNumber);

		SpoofSerialNumber(SerialNumber);

		//DbgPrint("%s: Spoofed: %s (%i)\n", __FUNCTION__,
		//	SerialNumber, strlen(SerialNumber));
	}
	else
	{
		//DbgPrint("%s: Invalid PSTORAGE_DEVICE_DESCRIPTOR\n", __FUNCTION__);
	}

	// Call next completion routine (if any)
	if ((Irp->StackCount > (ULONG)1) && (OldCompletionRoutine != NULL))
		return OldCompletionRoutine(DeviceObject, Irp, OldContext);

	return STATUS_SUCCESS;
}

NTSTATUS HookedMjDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	PIO_STACK_LOCATION Ioc = IoGetCurrentIrpStackLocation(Irp);

	switch (Ioc->Parameters.DeviceIoControl.IoControlCode)
	{
	case IOCTL_STORAGE_QUERY_PROPERTY:
	{
		PSTORAGE_PROPERTY_QUERY query = (PSTORAGE_PROPERTY_QUERY)Irp->AssociatedIrp.SystemBuffer;

		if (query->PropertyId == StorageDeviceProperty)
		{
			// Register CompletionRotuine
			Ioc->Control = 0;
			Ioc->Control |= SL_INVOKE_ON_SUCCESS;

			// Save old completion routine
			PVOID OldContext = Ioc->Context;
			Ioc->Context = (PVOID)ExAllocatePool(NonPagedPool, sizeof(REQUEST_STRUCT));
			REQUEST_STRUCT* pRequest = (REQUEST_STRUCT*)Ioc->Context;
			pRequest->OldRoutine = Ioc->CompletionRoutine;
			pRequest->OldContext = OldContext;
			pRequest->OutputBufferLength = Ioc->Parameters.DeviceIoControl.OutputBufferLength;
			pRequest->StorageDescriptor = (PSTORAGE_DEVICE_DESCRIPTOR)Irp->AssociatedIrp.SystemBuffer;

			// Setup our function to be called
			// upon completion of the IRP
			Ioc->CompletionRoutine = (PIO_COMPLETION_ROUTINE)StorageQueryCompletionRoutine;
		}

		break;

	}
	case SMART_RCV_DRIVE_DATA:
	{
		Ioc->Control = 0;
		Ioc->Control |= SL_INVOKE_ON_SUCCESS;

		// Save old completion routine
		PVOID OldContext = Ioc->Context;
		Ioc->Context = (PVOID)ExAllocatePool(NonPagedPool, sizeof(REQUEST_STRUCT));
		REQUEST_STRUCT* pRequest = (REQUEST_STRUCT*)Ioc->Context;
		pRequest->OldRoutine = Ioc->CompletionRoutine;
		pRequest->OldContext = OldContext;

		// Setup our function to be called
		// upon completion of the IRP
		Ioc->CompletionRoutine = (PIO_COMPLETION_ROUTINE)SmartCompletionRoutine;

		break;
	}
	}

	return OldIrpMj(DeviceObject, Irp);

}

NTSTATUS IrpHookDisk()
{
	PDRIVER_OBJECT hookDriver = NULL;

	UNICODE_STRING unDriverName;
	RtlInitUnicodeString(&unDriverName, L"\\Driver\\Disk");

	auto Status = ObReferenceObjectByName(&unDriverName,
		OBJ_CASE_INSENSITIVE, NULL, 0, *IoDriverObjectType, KernelMode, NULL,
		(PVOID*)&hookDriver);

	if (!NT_SUCCESS(Status))
	{
		DbgPrint("Failed to get driver object ptr: %X\n", Status);
		return Status;
	}

	OldIrpMj = hookDriver->MajorFunction[IRP_MJ_DEVICE_CONTROL];
	hookDriver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = (PDRIVER_DISPATCH)HookedMjDeviceControl;

	return Status;

}
