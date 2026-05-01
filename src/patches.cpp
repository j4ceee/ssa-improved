#include "patches.h"
#include "log.h"
#include "MinHook.h"
#include "graphics/d3d9_hooks.h"
#include "graphics/window_hooks.h"
#include "libusb/portal_device.h"
#include "libusb/hid.h"
#include "libusb/kernel.h"
#include "libusb/setupapi.h"

namespace ssa
{

    bool InitPatchesAndHooks()
    {
        Log("Starting InitPatchesAndHooks");

        // detect WinUSB driver presence and initialise libusb context
        // this must happen before hooks are installed so SetupAPI injection knows whether to activate
        if (!Portal::Init()) {
            Log("Portal::Init failed - continuing without libusb support");
        }

        if (MH_Initialize() != MH_OK) {
            Log("Failed to initialize MinHook");
            return false;
        }

        try {
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

            if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK)
                throw std::runtime_error("MH_EnableHook failed");

        } catch (const std::exception& e) {
            Log("Exception during patch init: %s", e.what());
            return false;
        }

        Log("All hooks enabled - mode: %s",
            Portal::g_winusb_mode ? "WinUSB (libusb injection active)"
                                  : "HID driver (pass-through)");
        return true;
    }

} // namespace ssa