#include <ntddk.h>
#include "shared.h"
#include "smbios.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h> 
/*

all of these comments here are when i was either high or insanely sleep deprived

*/

#define MAX_SMBIOS_TYPES 256 

char* GetString(SMBIOS_HEADER* header, SMBIOS_STRING string) {
	if (header == NULL) {
		DbgPrint("GetString: header is NULL\n");
		return NULL;
	}

	const char* start = (const char*)header + header->Length;

	if (!string || *start == 0) {
		return NULL;
	}

	while (--string) {
		start += strlen(start) + 1;
		if (*start == 0) {
			return NULL;
		}
	}

	return (char*)start;
}
GUID* GetUUID(SMBIOS_HEADER* header) {
	if (header == NULL) {
		DbgPrint("GetUUID: header is NULL\n");
		return NULL;
	}

	SMBIOS_TYPE1* type1 = (SMBIOS_TYPE1*)header;
	return &(type1->UUID);
}
void RandomizeUUID(GUID* uuid) {

	if (uuid == NULL) {
		DbgPrint("UUID is NULL, skipping randomization.\n");
		return;
	}

	// UUID is always 16 bytes
	int length = 16;
	UINT8* buffer = (UINT8*)ExAllocatePoolWithTag(NonPagedPool, length * sizeof(UINT8), POOL_TAG);
	if (buffer != NULL) {
		for (int i = 0; i < length; ++i) {
			buffer[i] = (UINT8)(rand() % 256);
		}

		//new uuid
		memcpy(uuid, buffer, length);

		//free tag out the trap
		ExFreePoolWithTag(buffer, POOL_TAG);
	}
	else {
		DbgPrint("Failed to allocate memory with tag" + POOL_TAG);
		DbgPrint("\n");
	}
}

void RandomizeString(char* string) {
	if (string == NULL) {
		DbgPrint("String is NULL, skipping randomization.\n");
		return;
	}

	int length = (int)strlen(string);

	// aw man the tag just got jailed :(
	char* buffer = (char*)ExAllocatePoolWithTag(NonPagedPool, length * sizeof(char), POOL_TAG);
	if (buffer != NULL) {
		Utils_RandomText(buffer, length);
		buffer[length] = '\0';

		memcpy(string, buffer, length);

		//dammit fuck this tag i hope it stays in jail :sob:
		ExFreePoolWithTag(buffer, POOL_TAG);
	}
	else {
		DbgPrint("Failed to allocate memory with tag 'lowk'\n");
	}
}


NTSTATUS ProcessTable(SMBIOS_HEADER* header) {
	if (header == NULL) {
		DbgPrint("ProcessTable: header is NULL\n");
		return STATUS_UNSUCCESSFUL;
	}

	if (!header->Length) {
		DbgPrint("ProcessTable: header length is zero\n");
		return STATUS_UNSUCCESSFUL;
	}

	DbgPrint("Processing SMBIOS header: Type=%d, Length=%d\n", header->Type, header->Length);

	switch (header->Type) {
	case 0: {
		SMBIOS_TYPE0* type0 = (SMBIOS_TYPE0*)header;

		char* vendor = GetString(header, type0->Vendor);
		if (vendor) {
			RandomizeString(vendor);
		}
		else {
			DbgPrint("ProcessTable: Vendor string is NULL\n");
		}
		char* version = GetString(header, type0->BiosVersion);
		if (version) {
			RandomizeString(version);
		}
		else {
			DbgPrint("error");
		}
		break;
	}
	case 1: {
		SMBIOS_TYPE1* type1 = (SMBIOS_TYPE1*)header;

		char* manufacturer = GetString(header, type1->Manufacturer);
		if (manufacturer) {
			RandomizeString(manufacturer);
			DbgPrint("manufactuer is changed woo hoo\n");
		}
		else {
			DbgPrint("ProcessTable: Manufacturer string is NULL\n");
		}
		char* productName = GetString(header, type1->ProductName);
		if (productName) {
			RandomizeString(productName);
			DbgPrint("product name is changed woo hoo\n");
		}
		else {
			DbgPrint("ProcessTable: ProductName string is NULL\n");
		}
		
		char* serialNumber = GetString(header, type1->SerialNumber);
		if (serialNumber) {
			RandomizeString(serialNumber);
			DbgPrint("serialnumber is changed woo hoo\n");
		}
		else {
			DbgPrint("ProcessTable: SerialNumber string is NULL\n");
		}
		char* version = GetString(header, type1->Version);
		if (version) {
			RandomizeString(version);
			DbgPrint("version changed\n");
		}
		else {
			DbgPrint("type 1 version is null, or something");
		}
		char* skunb = GetString(header, type1->SKUNumber);
		if (skunb) {
			RandomizeString(skunb);
			DbgPrint("sku number changed\n");
		}
		GUID* uuid = GetUUID(header);
		if (uuid) {
			RandomizeUUID(uuid);
                  DbgPrint("UUID is changed woo hoo\n");
            } else {
                DbgPrint("ProcessTable: UUID is NULL\n");
            }
		char* fam = GetString(header, type1->Family);
		if (fam) {
			RandomizeString(fam);
			DbgPrint("family changed "); // orphanized or foster care whichever one lowk
		}
		break;
	}
	case 2: {
		SMBIOS_TYPE2* type2 = (SMBIOS_TYPE2*)header;

		char* manufacturer = GetString(header, type2->Manufacturer);
		if (manufacturer) {
			RandomizeString(manufacturer);
			DbgPrint("manufactuer is changed woo hoo\n");
		}
		else {
			DbgPrint("ProcessTable: Manufacturer string is NULL\n");
		}

		char* productName = GetString(header, type2->ProductName);
		if (productName) {
			RandomizeString(productName);
			DbgPrint("product name is changed woo hoo\n");
		}
		else {
			DbgPrint("ProcessTable: ProductName string is NULL\n");
		}

		char* serialNumber = GetString(header, type2->SerialNumber);
		if (serialNumber) {
			RandomizeString(serialNumber);
			DbgPrint("serialnumber is changed woo hoo\n");
		}
		else {
			DbgPrint("ProcessTable: SerialNumber string is NULL\n");
		}
		char* asstag = GetString(header, type2->AssetTag); 
		if (asstag) {
			RandomizeString(asstag);
		}
		char* chasloc = GetString(header, type2->LocationWithinContainer);
		if (chasloc) {
			RandomizeString(chasloc);
		}
		else {
			DbgPrint("unspecified error!");
		}
		char* version = GetString(header, type2->Version);
		if (version) {
			DbgPrint("changed!\n");
			RandomizeString(version);
		}
		else{
			DbgPrint("Version is null... somehow.\n");
		}
		break;
	}
	case 3: {
		SMBIOS_TYPE3* type3 = (SMBIOS_TYPE3*)header;

		char* manufacturer = GetString(header, type3->Manufacturer);
		if (manufacturer) {
			DbgPrint("manufactuer is changed woo hoo\n");
			RandomizeString(manufacturer);
		}
		else {
			DbgPrint("ProcessTable: Manufacturer string is NULL\n");
		}

		char* serialNumber = GetString(header, type3->SerialNumber);
		if (serialNumber) {
			RandomizeString(serialNumber);
		}
		else {
			DbgPrint("ProcessTable: SerialNumber string is NULL\n");
		}
		break;
	}
	case 4: {
		SMBIOS_TYPE4* type4 = (SMBIOS_TYPE4*)header;

		char* serial = GetString(header, type4->SerialNumber);
		if (serial) {
			DbgPrint("changing processer serial number\n");
			RandomizeString(serial);
		}
		else {
			DbgPrint("serial is null\n");
		}
	}
	default:
		break;
	}

	return STATUS_SUCCESS;
}

NTSTATUS LoopTables(void* mapped, ULONG size) {
	char* endAddress = (char*)mapped + size;
	bool processedTypes[MAX_SMBIOS_TYPES] = { false };  // Initialize all types as not processed

	while (1) {
		SMBIOS_HEADER* header = (SMBIOS_HEADER*)mapped;

		DbgPrint("Processing SMBIOS header: Type=%d, Length=%d\n", header->Type, header->Length);

		// Check for end of SMBIOS tables
		if (header->Type == 127 && header->Length == 4) {
			DbgPrint("Reached end of SMBIOS tables.\n");
			break;
		}

		// Error handling if type is 255
		if (header->Type == 255) {
			DbgPrint("ERROR FOR SOME REASON");
			break;
		}

		if (processedTypes[header->Type]) {
			DbgPrint("");
		}
		else {
			NTSTATUS status = ProcessTable(header);
			if (!NT_SUCCESS(status)) {
				DbgPrint("Failed to process SMBIOS table: Status=0x%x\n", status);
				return status;
			}

			processedTypes[header->Type] = true;
		}
		char* currentAddress = (char*)mapped + header->Length;

		while (currentAddress < endAddress - 1 && !(currentAddress[0] == 0 && currentAddress[1] == 0)) {
			currentAddress++;
		}

		if (currentAddress < endAddress - 1) {
			currentAddress += 2;
		}
		else {
	
			break;
		}

		if (currentAddress >= endAddress) {
			DbgPrint("Reached end of mapped memory.\n");
			break;
		}

		mapped = currentAddress;
	}

	return STATUS_SUCCESS;
}

NTSTATUS ChangeSmbiosSerials()
{
	PVOID base = Utils_GetModuleBase("ntoskrnl.exe");
	if (!base)
	{
		DbgPrint("Failed to find ntoskrnl.sys base!\n");
		return STATUS_UNSUCCESSFUL;
	}

	PPHYSICAL_ADDRESS physicalAddress = (PPHYSICAL_ADDRESS)Utils_FindPatternImage(base, "\x48\x8B\x0D\x00\x00\x00\x00\x48\x85\xC9\x74\x00\x8B\x15", "xxx????xxxx?xx"); // WmipFindSMBiosStructure -> WmipSMBiosTablePhysicalAddress
	if (!physicalAddress)
	{
		DbgPrint("Failed to find SMBIOS physical address!\n");
		return STATUS_UNSUCCESSFUL;
	}

	DbgPrint("physicalAddress:", physicalAddress);
	physicalAddress = (PPHYSICAL_ADDRESS)((char*)physicalAddress + 7 + *(int*)((char*)physicalAddress + 3));
	if (!physicalAddress)
	{
		DbgPrint("Physical address is null!\n");
		return STATUS_UNSUCCESSFUL;
	}
	//finally.
	PVOID sizeScan = Utils_FindPatternImage(base, "\x8B\x1D\x00\x00\x00\x00\x48\x8B\xD0\x44\x8B\xC3\x48\x8B\xCD\xE8\x00\x00\x00\x00\x8B\xD3\x48\x8B", "xx????xxxxxxxxxx????xxxx");    // WmipFindSMBiosStructure -> WmipSMBiosTableLength
	if (!sizeScan)
	{
		DbgPrint("Failed to find SMBIOS size! \n");
		return STATUS_UNSUCCESSFUL;
	}

	ULONG size = *(ULONG*)((char*)sizeScan + 6 + *(int*)((char*)sizeScan + 2));
	if (!size)
	{
		DbgPrint("SMBIOS size is null!\n");
		return STATUS_UNSUCCESSFUL;
	}

	PVOID mapped = MmMapIoSpace(*physicalAddress, size, MmNonCached);
	if (!mapped)
	{
		DbgPrint("Failed to map SMBIOS structures!\n");
		return STATUS_UNSUCCESSFUL;
	}

	LoopTables(mapped, size);

	MmUnmapIoSpace(mapped, size);

	return STATUS_SUCCESS;
}