#include <Windows.h>

#include "addresses.h"
#include "xinput1_3.h"
#include "log.h"
#include "MinHook.h"
#include "patches.h"
// #include "hooks.h"

static HMODULE g_original_module = nullptr;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH: {
            if (!GetModuleHandle("Skylanders.exe"))
            {
                ssa::Log("Not in SSA - exiting");
                return TRUE;
            }

            ssa::Log("DLL injected successfully into SSA!");

            // load the real XInput DLL
            char buffer[MAX_PATH] = {0};
            GetSystemDirectoryA(buffer, MAX_PATH);
            strcat_s(buffer, MAX_PATH, "\\xinput1_3.dll");

            ssa::Log("Loading original XInput DLL from: %s", buffer);

            g_original_module = LoadLibraryA(buffer);
            if (g_original_module) {
                XInputGetState_ = (XInputGetState_t)GetProcAddress(g_original_module, "XInputGetState");
                XInputSetState_ = (XInputSetState_t)GetProcAddress(g_original_module, "XInputSetState");
                XInputEnable_ = (XInputEnable_t)GetProcAddress(g_original_module, "XInputEnable");
                XInputGetCapabilities_ = (XInputGetCapabilities_t)GetProcAddress(g_original_module, "XInputGetCapabilities");
                ssa::Log("XInput proxy loaded successfully");
            } else {
                ssa::Log("Failed to load original XInput DLL");
            }

            ssa::InitAddresses();
            if (!ssa::InitPatchesAndHooks()) {
                ssa::Log("Failed to initialize patches and hooks - exiting");
                MessageBox(nullptr, "There was an error setting up the mod. This should not happen."
                    "\nMake sure you have the latest and unmodified version of the mod and the game.",
                    "Skylanders Spyros's Adventure", MB_OK | MB_ICONEXCLAMATION);
                return FALSE;
            }
            ssa::Log("Successfully initialized patches and hooks ");

            break;
        }

        case DLL_PROCESS_DETACH: {
            ssa::Log("DLL unloading");
            MH_DisableHook(MH_ALL_HOOKS);
            MH_Uninitialize();
            if (g_original_module) {
                FreeLibrary(g_original_module);
                g_original_module = nullptr;
            }
            break;
        }
    }

    return TRUE;
}
