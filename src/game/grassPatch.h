#pragma once
#include "config.h"
#include "hook_helpers.h"

namespace ssa::Game::GrassPatch
{
    using GrassDrawAll_t = void(*)();
    inline GrassDrawAll_t orig_GrassDrawAll = nullptr;

    inline void hook_GrassDrawAll() {
        if (g_config.disableGrass) {
            // skip original function, effectively preventing grass from rendering
            return;
        }

        orig_GrassDrawAll();
    }

    inline bool HookGrassDrawAll()
    {
        if (orig_GrassDrawAll != nullptr) {
            return true; // already set up
        }
        return MH_CreateHookSSA(GRASS_DRAW_ALL, &hook_GrassDrawAll, &orig_GrassDrawAll);
    }
}