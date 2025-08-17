#include <ntifs.h>
#include <windef.h>
#include <ntimage.h>
#include "shared.h"
#include "utils.h"

/**
 * \brief Get base address of kernel module
 * \param moduleName Name of the module (ex. storport.sys)
 * \return Address of the module or null pointer if failed
 */
PVOID Utils_GetModuleBase(const char* moduleName)
{
    PVOID address = NULL;
    ULONG size = 0;

    NTSTATUS status = ZwQuerySystemInformation(SystemModuleInformation, &size, 0, &size);
    if (status != STATUS_INFO_LENGTH_MISMATCH)
        return NULL;

    PSYSTEM_MODULE_INFORMATION moduleList = (PSYSTEM_MODULE_INFORMATION)ExAllocatePoolWithTag(NonPagedPool, size, POOL_TAG);
    if (!moduleList)
        return NULL;

    status = ZwQuerySystemInformation(SystemModuleInformation, moduleList, size, NULL);
    if (!NT_SUCCESS(status))
        goto end;

    for (ULONG i = 0; i < moduleList->ulModuleCount; i++)
    {
        SYSTEM_MODULE module = moduleList->Modules[i];
        if (strstr(module.ImageName, moduleName))
        {
            address = module.Base;
            break;
        }
    }

end:
    ExFreePool(moduleList);
    return address;
}


/**
 * \brief Checks if buffer at the location of base parameter
 * matches pattern and mask
 * \param base Address to check
 * \param pattern Byte pattern to match
 * \param mask Mask containing unknown bytes
 * \return
 */
bool Utils_CheckMask(const char* base, const char* pattern, const char* mask)
{
    for (; *mask; ++base, ++pattern, ++mask)
    {
        if ('x' == *mask && *base != *pattern)
        {
            return FALSE;
        }
    }

    return TRUE;
}

/**
 * \brief Find byte pattern in given buffer
 * \param base Address to start searching in
 * \param length Maximum length
 * \param pattern Byte pattern to match
 * \param mask Mask containing unknown bytes
 * \return Pointer to matching memory
 */
PVOID Utils_FindPattern(PVOID base, int length, const char* pattern, const char* mask)
{
    length -= (int)strlen(mask);
    for (int i = 0; i <= length; ++i)
    {
        const char* data = (char*)base;
        const char* address = &data[i];
        if (Utils_CheckMask(address, pattern, mask))
            return (PVOID)address;
    }

    return NULL;
}

/**
 * \brief Find byte pattern in given module/image ".text" and "PAGE" sections
 * \param base Base address of the kernel module
 * \param pattern Byte pattern to match
 * \param mask Mask containing unknown bytes
 * \return Pointer to matching memory
 */
PVOID Utils_FindPatternImage(PVOID base, const char* pattern, const char* mask)
{
    PVOID match = NULL;

    PIMAGE_NT_HEADERS headers = (PIMAGE_NT_HEADERS)((char*)base + ((PIMAGE_DOS_HEADER)base)->e_lfanew);
    PIMAGE_SECTION_HEADER sections = IMAGE_FIRST_SECTION(headers);

    for (USHORT i = 0; i < headers->FileHeader.NumberOfSections; ++i)
    {
        PIMAGE_SECTION_HEADER section = &sections[i];
        if (*((PINT)section->Name) == 'EGAP' || memcmp(section->Name, ".text", 5) == 0)
        {
            match = Utils_FindPattern((char*)base + section->VirtualAddress, section->Misc.VirtualSize, pattern, mask);
            if (match)
                break;
        }
    }

    return match;
}

/**
 * \brief Generate pseudo-random text into given buffer
 * \param text Pointer to text
 * \param length Desired length
 */
void Utils_RandomText(char* text, int length)
{
    if (!text)
        return;

    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    ULONG seed = KeQueryTimeIncrement();

    for (int n = 0; n <= length; n++)
    {
        int key = RtlRandomEx(&seed) % (sizeof(alphanum) - 1);
        text[n] = alphanum[key];
    }
}

