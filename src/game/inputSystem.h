#pragma once
#include "hook_helpers.h"
#include "log.h"
#include "../imgui/ui.h"

namespace ssa::Game::EngineInputHooks
{
    using NakedHook_t = void(*)();

    inline NakedHook_t orig_UpdateMouse = nullptr;
    inline NakedHook_t orig_UpdateKeyboard = nullptr;
    inline NakedHook_t orig_UpdateController = nullptr;

    // helper to cleanly check UI state without C++ trashing registers in our naked hooks
    static bool __stdcall ShouldBlockInput()
    {
        return UI::Get()->IsVisible();
    }

    // ---------------------------------------------------------
    // naked hooks: preserves EAX, ESI, ECX, ...
    // ---------------------------------------------------------

    inline __declspec(naked) void hook_UpdateMouse()
    {
        __asm {
            pushad                              // save all registers
            call ShouldBlockInput               // call C++ helper
            test al, al                         // check if returned true
            jnz Blocked                         // if true, jump to Blocked

            popad                               // restore ALL registers
            jmp dword ptr [orig_UpdateMouse]    // jump directly to the original engine code

        Blocked:
            popad                               // restore all registers
            xor eax, eax                        // set EAX to 0 (return false)
            ret 4                               // return and clean up the 4-byte argument on the stack
        }
    }

    inline __declspec(naked) void hook_UpdateKeyboard()
    {
        __asm {
            pushad
            call ShouldBlockInput
            test al, al
            jnz Blocked

            popad
            jmp dword ptr [orig_UpdateKeyboard]

        Blocked:
            popad
            xor eax, eax
            ret 4
        }
    }

    inline __declspec(naked) void hook_UpdateController()
    {
        __asm {
            pushad
            call ShouldBlockInput
            test al, al
            jnz Blocked

            popad
            jmp dword ptr [orig_UpdateController]

        Blocked:
            popad
            xor eax, eax
            ret 4
        }
    }

    inline bool InitInputHooks()
    {
        if (!MH_CreateHookSSA(UPDATE_MOUSE, &hook_UpdateMouse, &orig_UpdateMouse)) {
            Log("[EngineInput] Failed to hook Mouse Update");
            return false;
        }

        if (!MH_CreateHookSSA(UPDATE_KEYBOARD, &hook_UpdateKeyboard, &orig_UpdateKeyboard)) {
            Log("[EngineInput] Failed to hook Keyboard Update");
            return false;
        }

        if (!MH_CreateHookSSA(UPDATE_CONTROLLER, &hook_UpdateController, &orig_UpdateController)) {
            Log("[EngineInput] Failed to hook Controller Update");
            return false;
        }

        Log("[EngineInput] Native engine hooks installed successfully!");
        return true;
    }
}