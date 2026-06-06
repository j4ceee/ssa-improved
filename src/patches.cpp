#include "patches.h"
#include "log.h"
#include "MinHook.h"
#include "window/d3d9_hooks.h"
#include "window/window_hooks.h"
#include "portal/portal_device.h"
#include "portal/hid.h"
#include "portal/kernel.h"
#include "portal/setupapi.h"
#include "input/winmm_hooks.h"
#include "input/dinput_hooks.h"

namespace ssa
{
    // hook all low level functions
    bool InitStartupHooks()
    {
        Log("[Hooks] Starting InitStartupHooks");

        // Portal initialization moved to hook_SetupDiEnumDeviceInterfaces (libusb does not work properly in DllMain
        // if (!Portal::Init())
        //     Log("[Hooks] Portal::Init failed: No Portal available, make sure a portal is plugged in or the emulated portal is active!");

        if (MH_Initialize() != MH_OK)
        {
            Log("[Hooks] Failed to initialize MinHook");
            return false;
        }

        try
        {
            // --- HOOKS START ---

            if (!KernelHooks::InitKernelHooks())
                throw std::runtime_error("InitKernelHooks failed");

            if (!SetupApiHooks::InitSetupApiHooks())
                throw std::runtime_error("SetupApiHooks failed");

            if (!HidHooks::InitHidHooks())
                throw std::runtime_error("HidHooks failed");

            if (!WindowHooks::InitWindowHooks())
                throw std::runtime_error("WindowHooks failed");

            if (!D3D9Hooks::InitD3D9Hooks())
                throw std::runtime_error("D3D9Hooks failed");

            // if (!WinmmHooks::InitWinmmHooks())
            //     Log("[Hooks] WinmmHooks failed, controller input will not be blocked");

            // --- HOOKS END ---

            if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK)
                throw std::runtime_error("MH_EnableHook failed");

        }
        catch (const std::exception& e)
        {
            Log("[Hooks] Exception during patch init: %s", e.what());
            return false;
        }

        Log("[Hooks] All hooks enabled");

        return true;
    }


    // SecuROM does not like hooking game functions before startup, instead let's do it during the first rendered frame
    void InitDeferredHooks()
    {
        static int attempts = 0;
        Log("[Hooks] Starting InitGameHooks, attempt #%d", ++attempts);

        static bool s_dinput_hooked = false;
        static bool s_winmm_hooked = false;

        try
        {
            if (!s_dinput_hooked)
            {
                s_dinput_hooked = DInputHooks::HookDInputMouseDevice();
                if (!s_dinput_hooked)
                    Log("[Hooks] Failed to hook DirectInput mouse device, mouse input may not be blocked when UI is open");
            }

            if (!s_winmm_hooked)
            {
                s_winmm_hooked = WinmmHooks::InitWinmmHooks();
                 if (!s_winmm_hooked)
                     Log("[Hooks] Failed to hook winmm joyGetPosEx, controller input may not be blocked when UI is open");
            }
        }
        catch (const std::exception& e)
        {
            Log("[Hooks] Exception during patch init: %s", e.what());
        }

        if (s_dinput_hooked && s_winmm_hooked)
            g_deferredHooksActive = true;
    }

} // namespace ssa