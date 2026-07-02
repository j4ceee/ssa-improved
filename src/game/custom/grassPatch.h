#pragma once
#include "addresses.h"
#include "config.h"

namespace ssa::Game::GrassPatch
{
    inline void ApplyGrassPatch()
    {
        if (g_config.disableGrass)
            *static_cast<int*>(GetAddress(GRASS_COUNT)) = 0;
    }
}