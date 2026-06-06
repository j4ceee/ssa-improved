#pragma once
#include <Windows.h>
#include <mmsystem.h>
#include "MinHook.h"
#include "log.h"
#include "imgui/ui.h"

namespace ssa::WinmmHooks
{
    using joyGetPosEx_t = MMRESULT(WINAPI*)(UINT, LPJOYINFOEX);
    inline joyGetPosEx_t orig_joyGetPosEx = nullptr;

    // returning any nonzero value causes FUN_009475f0 to goto past the entire controller input block
    inline MMRESULT WINAPI hook_joyGetPosEx(UINT uJoyID, LPJOYINFOEX pji)
    {
        if (UI::Get()->IsVisible())
            return JOYERR_PARMS;
        return orig_joyGetPosEx(uJoyID, pji);
    }

    inline bool InitWinmmHooks()
    {
        void* pTarget = nullptr;
        MH_STATUS s = MH_CreateHookApiEx(L"winmm.dll", "joyGetPosEx", &hook_joyGetPosEx, (void**)&orig_joyGetPosEx, &pTarget);

        if (s == MH_OK) {
            if (MH_EnableHook(pTarget) == MH_OK)
            {
                Log("[Winmm] Hooked joyGetPosEx");
                return true;
            }
        }

        Log("[Winmm] Failed to hook joyGetPosEx: %d", s);
        return false;
    }
}