#pragma once

namespace ssa
{
    bool InitStartupHooks();
    void InitGameHooks();
    inline bool g_gameHooksActive = false;
}
