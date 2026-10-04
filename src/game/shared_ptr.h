#pragma once

#include <Windows.h>
#include <cstdint>
#include <meow_hook/util.h>
#include "addresses.h"
#include "data_types.h"

// Reference counting of the engine's ltl2::shared_ptr / weak_ptr, for code that has to hold references itself
// counts live in control block; last release calls back into the game
namespace ssa::Game::Shared
{
    inline SharedControl* Control(void* ctrl) { return static_cast<SharedControl*>(ctrl); }

    // works with SharedPtr<T> and WeakPtr<T>: true while a strong reference keeps the object alive
    template <class Ref>
    [[nodiscard]] bool IsAlive(const Ref& ref)
    {
        return ref.ptr && ref.ctrl && Control(ref.ctrl)->strong > 0;
    }

    inline void AddStrong(void* ctrl)
    {
        InterlockedIncrement(&Control(ctrl)->strong);
    }

    inline void AddWeak(void* ctrl)
    {
        InterlockedIncrement(&Control(ctrl)->weak);
    }

    inline void ReleaseStrong(void* ctrl)
    {
        auto* block = Control(ctrl);
        if (InterlockedDecrement(&block->strong) == 0)
            meow_hook::func_call<void>(reinterpret_cast<uintptr_t>(GetAddress(SHARED_CTRL_ON_STRONG_ZERO)), block);
    }

    inline void ReleaseWeak(void* ctrl)
    {
        if (!ctrl) return;

        auto* block = Control(ctrl);
        if ((static_cast<uint32_t>(InterlockedDecrement(&block->weak)) & 0x7fffffffu) == 0)
            meow_hook::func_call<void>(reinterpret_cast<uintptr_t>(GetAddress(SHARED_CTRL_ON_WEAK_ZERO)), block);
    }
} // namespace ssa::Game::Shared
