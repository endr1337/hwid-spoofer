#pragma once
#include <stdbool.h>

	PVOID Utils_GetModuleBase(const char* moduleName);
	bool Utils_CheckMask(const char* base, const char* pattern, const char* mask);
	PVOID Utils_FindPattern(PVOID base, int length, const char* pattern, const char* mask);
	PVOID Utils_FindPatternImage(PVOID base, const char* pattern, const char* mask);
	void Utils_RandomText(char* text, const int length);
