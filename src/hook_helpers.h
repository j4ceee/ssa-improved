#pragma once

#include "addresses.h"
#include "MinHook.h"

namespace ssa
{
    /// <summary>
    /// Helper function to create a hook with MinHook and get the original function pointer.
    /// </summary>
    /// <param name="targetAddress">The target function address to hook.</param>
    /// <param name="detourFunction">The detour function address.</param>
    /// <param name="ppOriginal">Pointer to store the original function pointer.</param>
    /// <returns>MH_STATUS indicating success or failure.</returns>
    ///
    template <typename T>
    bool MH_CreateHookSSA(Address targetAddress, T* detourFunction, T** ppOriginal)
    {
        void* address = GetAddress(targetAddress);
        MH_STATUS result = MH_CreateHook(
                    reinterpret_cast<LPVOID>(address),
                    reinterpret_cast<LPVOID>(detourFunction),
                    reinterpret_cast<LPVOID*>(ppOriginal)
                );

        if (result != MH_OK) {
            return false;
        }

        MH_STATUS enableResult = MH_EnableHook(
                    reinterpret_cast<LPVOID>(address)
                );

        if (enableResult != MH_OK) {
            return false;
        }

        return true;
    }
} // namespace ssa