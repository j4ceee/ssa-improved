#pragma once

#include <atomic>
#include <cstdint>
#include "config.h"
#include "game/data/levels.h"
#include "game/game.h"

// Hides the normal in-game UI by not executing the draw calls that paint it
namespace ssa::UiHider
{
    // FNV-1a of the scene composite shader's bytecode (bloom + colour grading)
    inline constexpr uint64_t kCompositeShaderHash = 0x320AF6A4F390FC1Bull;

    inline std::atomic<const void*> g_compositeShader{nullptr}; // game's composite shader object
    inline std::atomic<const void*> g_rt0{nullptr}; // surface currently bound as render target 0
    inline std::atomic<const void*> g_finalTarget{nullptr}; // surface the scene was composited into this frame
    inline std::atomic<bool> g_hiding{false}; // set by the composite draw, cleared every Present

    // true while the mod's own ImGui overlay is drawing, it must never be hidden
    inline thread_local bool g_inOverlay = false;

    // call from ShaderTracker::OnPixelShaderCreated
    inline void OnShaderCreated(const void* shader, uint64_t hash)
    {
        if (hash == kCompositeShaderHash)
            g_compositeShader = shader;
        else if (g_compositeShader.load() == shader)
            g_compositeShader = nullptr; // the address was reused by another shader
    }

    // called from every draw hook with the pixel shader that is currently set; true = do not execute this draw call
    inline bool ShouldSkipDraw(const void* currentPS)
    {
        if (!g_config.hideUi || g_inOverlay) return false;

        const void* rt = g_rt0.load(std::memory_order_relaxed);

        if (currentPS && currentPS == g_compositeShader.load(std::memory_order_relaxed))
        {
            // the scene was just composited into the final image: from now on, draws into it are UI
            const Game::Game* game = Game::Game::instance();
            const bool inGameplay = game->m_CurrLevel != Game::Data::Levels::FrontEnd.crc;
            g_finalTarget.store(rt, std::memory_order_relaxed);
            g_hiding.store(inGameplay, std::memory_order_relaxed);
            return false;
        }

        return rt && g_hiding.load(std::memory_order_relaxed) && rt == g_finalTarget.load(std::memory_order_relaxed);
    }

    // called once per frame from Present
    inline void OnPresent()
    {
        g_hiding.store(false, std::memory_order_relaxed);
    }

    // called from hook_Reset
    inline void OnReset()
    {
        g_hiding.store(false, std::memory_order_relaxed);
        g_rt0.store(nullptr, std::memory_order_relaxed);
        g_finalTarget.store(nullptr, std::memory_order_relaxed);
    }
} // namespace ssa::UiHider
