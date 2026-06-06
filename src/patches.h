#pragma once

namespace ssa
{
    bool InitStartupHooks();
    void InitDeferredHooks();
    inline bool g_deferredHooksActive = false;
}
