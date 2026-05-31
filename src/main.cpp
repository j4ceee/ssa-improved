#include <Windows.h>

#include "addresses.h"
#include "xinput1_3.h"
#include "log.h"
#include "MinHook.h"
#include "patches.h"
#include "graphics/window_hooks.h"
#include "imgui/ui.h"
#include "portal/portal_device.h"
#include "skylanders/SkylanderManager.h"

static HMODULE g_original_module = nullptr;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH: {
            if (!GetModuleHandle("Skylanders.exe"))
            {
                ssa::LogF("Not in SSA - exiting");
                return TRUE;
            }

            ssa::LogF("DLL injected successfully into SSA!");

            // load the real XInput DLL
            char buffer[MAX_PATH] = {0};
            GetSystemDirectoryA(buffer, MAX_PATH);
            strcat_s(buffer, MAX_PATH, "\\xinput1_3.dll");

            ssa::LogF("Loading original XInput DLL from: %s", buffer);

            g_original_module = LoadLibraryA(buffer);
            if (g_original_module) {
                XInputGetState_ = (XInputGetState_t)GetProcAddress(g_original_module, "XInputGetState");
                XInputSetState_ = (XInputSetState_t)GetProcAddress(g_original_module, "XInputSetState");
                XInputEnable_ = (XInputEnable_t)GetProcAddress(g_original_module, "XInputEnable");
                XInputGetCapabilities_ = (XInputGetCapabilities_t)GetProcAddress(g_original_module, "XInputGetCapabilities");
                ssa::LogF("XInput proxy loaded successfully");
            } else {
                ssa::LogF("Failed to load original XInput DLL");
            }

            ssa::LoadConfig(); // read file (or use defaults if absent)
            bool filesOk = ssa::InitConfigFile(); // write / create file, set g_configWritable

            if (!filesOk) {
                MessageBox(nullptr,
                    "The mod could not create its config and log files.\n"
                    "This is usually because the game folder requires administrator permissions."
                    "\n\nPlease run the game as Administrator at least once to allow the mod to set itself up or adjust permissions on the game folder."
                    "\n\nThe game will continue to run, but mod settings will use their defaults.",
                    "Skylanders Spyros's Adventure Improved",
                    MB_OK | MB_ICONWARNING);
            }

            ssa::InitAddresses();
            if (!ssa::InitStartupHooks()) {
                ssa::LogF("Failed to initialize patches and hooks - exiting");
                MessageBox(nullptr, "There was an error setting up the mod. This should not happen."
                    "\nMake sure you have the latest and unmodified version of the mod and the game.",
                    "Skylanders Spyros's Adventure Improved", MB_OK | MB_ICONEXCLAMATION);
                return FALSE;
            }
            ssa::LogF("Successfully initialized patches and hooks ");

            ssa::Skylanders::SkylanderManager::Get().Initialize();
            ssa::LogF("Skylander Manager initialized");

            break;
        }

        case DLL_PROCESS_DETACH: {
            ssa::LogF("DLL unloading");
            ssa::WindowHooks::ShutdownWindowHooks();
            MH_DisableHook(MH_ALL_HOOKS);
            MH_Uninitialize();
            ssa::UI::Get()->Shutdown();
            // close proxy socket cleanly on exit
            ssa::Portal::Shutdown();
            if (g_original_module) {
                FreeLibrary(g_original_module);
                g_original_module = nullptr;
            }
            break;
        }
    }

    return TRUE;
}
