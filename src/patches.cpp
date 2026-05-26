#include "patches.h"
#include "log.h"
#include "MinHook.h"
#include "game/grassPatch.h"
#include "graphics/d3d9_hooks.h"
#include "graphics/window_hooks.h"
#include "libusb/portal_device.h"
#include "libusb/hid.h"
#include "libusb/kernel.h"
#include "libusb/setupapi.h"

namespace ssa
{
    // hook all low level functions
    bool InitStartupHooks()
    {
        Log("[Hooks] Starting InitStartupHooks");

        // detect WinUSB driver presence and initialise libusb context
        // this must happen before hooks are installed so SetupAPI injection knows whether to activate
        if (!Portal::Init())
            Log("[Hooks] Portal::Init failed - continuing without libusb support");

        if (MH_Initialize() != MH_OK) {
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

            // --- HOOKS END ---

            if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK)
                throw std::runtime_error("MH_EnableHook failed");

        }
        catch (const std::exception& e)
        {
            Log("[Hooks] Exception during patch init: %s", e.what());
            return false;
        }

        Log("[Hooks] All hooks enabled - mode: %s",
            Portal::g_winusb_mode ? "WinUSB (libusb injection active)"
                                  : "HID driver (pass-through)");
        return true;
    }


    // SecuROM does not like hooking game functions before startup, instead let's do it during the first rendered frame
    void InitGameHooks()
    {
        Log("[Hooks] Starting InitGameHooks");

        try
        {
            if (!Game::GrassPatch::HookGrassDrawAll())
                Log("[Hooks] Failed to hook GrassDrawAll");
        }
        catch (const std::exception& e)
        {
            Log("[Hooks] Exception during patch init: %s", e.what());
        }

        g_gameHooksActive = true;
    }

} // namespace ssa