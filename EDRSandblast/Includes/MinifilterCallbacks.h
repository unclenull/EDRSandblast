#pragma once
#include <Windows.h>

BOOL EnumMinifilterCallbacks(struct FOUND_EDR_CALLBACKS* FoundObjectCallbacks);
void DisableMinifilterCallbacks(struct FOUND_EDR_CALLBACKS* FoundObjectCallbacks);
void EnableMinifilterCallbacks(struct FOUND_EDR_CALLBACKS* FoundObjectCallbacks);
