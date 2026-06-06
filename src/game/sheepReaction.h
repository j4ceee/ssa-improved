#pragma once
#include "hook_helpers.h"

namespace ssa::Game::SheepReaction
{
    // no game hooks with SecuROM & no workaround yet (if ever) for sheep :(
    using SheepReaction_Update_t = void(__fastcall*)(void* self, void* dummy_edx, float dt);
    inline SheepReaction_Update_t orig_SheepReactionUpdate = nullptr;

    constexpr float SHEEP_TIME_SCALE = 0.1f; // sheep stay in their modified form for 10x longer

    inline void __fastcall hook_SheepReactionUpdate(void* self, void*, float dt) {
        orig_SheepReactionUpdate(self, nullptr, dt * SHEEP_TIME_SCALE);
    }

    inline bool HookSheep()
    {
        if (orig_SheepReactionUpdate != nullptr) {
            return true; // already set up
        }
        return MH_CreateHookSSA(SHEEP, &hook_SheepReactionUpdate, &orig_SheepReactionUpdate);
    }
}