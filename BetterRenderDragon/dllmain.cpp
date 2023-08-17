﻿#include <Windows.h>
#include <wrl.h>
#pragma comment(lib, "runtimeobject.lib")

#include "ImGuiHooks.h"
#include "MCHooks.h"
#include "MCPatches.h"
#include "Options.h"

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	char fileName[4096] = { 0 };
	GetModuleFileNameA(GetModuleHandleA(nullptr), fileName, sizeof(fileName) - 1);
	int len = strlen(fileName);
	if (len < 21 || strncmp(fileName + len - 21, "Minecraft.Windows.exe", 21) != 0) {
		return TRUE;
	}

    switch (ul_reason_for_call) {
		case DLL_PROCESS_ATTACH: {
			RoInitialize(RO_INIT_MULTITHREADED);
			Options::init();
			Options::load();

			MCHooks_Init();
			MCPatches_Init();
			ImGuiHooks_Init();
			break;
		}
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
			break;
		case DLL_PROCESS_DETACH:
			Options::save();
			break;
    }
    return TRUE;
}
