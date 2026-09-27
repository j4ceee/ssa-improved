#pragma once
#include <algorithm>
#include <Windows.h>
#include "game/world.h"

namespace ssa::Game::LoadScreenFix
{
    // LoadScreen's thread advances its timers by frameCorrector * updateSpd per loop iteration
    // frameCorrector is stale while the main thread loads -> feed it the real loop period
    inline void Update()
    {
        static LARGE_INTEGER s_freq{}, s_last{};
        if (!s_freq.QuadPart) QueryPerformanceFrequency(&s_freq);

        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);

        float elapsed = 1.0f / 30.0f; // nominal loader period for the first tick of each load screen
        if (s_last.QuadPart) {
            const float e = float(now.QuadPart - s_last.QuadPart) / float(s_freq.QuadPart);
            if (e > 0.0f && e < 0.25f)
                elapsed = e;
        }
        s_last = now;

        auto* w = World::instance();
        if (!w || w->updateSpd <= 0.0f) return;

        w->frameCorrector = std::clamp(elapsed / w->updateSpd, 0.0f, 3.0f); // same clamp as the game
    }
}