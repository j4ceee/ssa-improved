#include "patches.h"
#include "addresses.h"
#include "log.h"
#include <iostream>
#include <__msvc_ostream.hpp>
#include "MinHook.h"
#include "libusb/kernel.h"


namespace ssa
{

bool InitPatchesAndHooks()
{
    Log("Starting InitPatchesAndHooks");

    if (MH_Initialize() != MH_OK)
    {
        Log("Failed to initialize MinHook");
        return false;
    }

    try {

        if (!KernelHooks::InitKernelHooks())
            throw std::runtime_error("InitKernelHooks failed");

    } catch (const std::exception &e) {
        Log("Exception during patch initialization: %s", e.what());
        return false;
    }

    return true;
}
} // namespace ssa
