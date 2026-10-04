#pragma once

#include <cstdint>
#include <string>
#include "config.h"
#include "shader_patcher.h"

// Replacement of the game's omni light pixel shader (see shader_patcher.h for how a patch is bound). The shader itself, with the
// tuning values of the shadows, is hlsl/omni_light.hlsl. Two independent features, both switched at runtime by constants set in
// PreDraw, so toggling them needs no rebuild:
// - screen-space shadows for dynamic lights (the game's own and the ones added by Game::AttackLights). The game only has a
//   shadow map for the sun, lights never cast shadows.
// - smoother light falloff: the game attenuates with 1 - (distance / radius)^2, which keeps lights bright almost to their rim
//   and then cuts off, so every light shows as a disc with an edge on the floor. A power curve gives a hotter core and a soft tail.
namespace ssa::OmniLightPatch
{
    // FNV-1a of the omni light shader's bytecode, as shown in the Pixel shaders tab
    inline constexpr uint64_t kOmniShaderHash = 0x679F608F065DDC26ull;

    // live tuning of the falloff (Diagnostics > Lighting shader tuning); copy values you like into the defaults
    struct Tuning
    {
        float falloffPower = 2.0f; // attenuation = (1 - (distance / radius)^2) ^ power; 1 = the game's falloff
        float falloffGain = 1.5f; // multiplies the result (a curve with power > 1 is darker than the game's, this brings it back)
    };

    inline Tuning g_tuning;

    // Per draw: c0..c3 of the omni light shader = mScreenToWorld, the shadows need the opposite direction (world -> screen) in
    // c100-c103 (ssa_mWorldToScreen); c112 (ssa_Light) holds the falloff values and the shadow switch. The hooks put the game's own
    // values back after the draw call.
    inline ShaderTweaks::Restore PreDraw(IDirect3DDevice9* dev)
    {
        ShaderTweaks::Restore applied;
        if (g_config.lightShadows)
            ShaderPatcher::UploadWorldToScreen(dev, 100, applied);

        const float values[4] = {
            g_tuning.falloffPower,
            g_tuning.falloffGain,
            g_config.lightShadows ? 1.0f : 0.0f, // shadows on
            0.0f,
        };
        ShaderTweaks::g_setPSConstF(dev, 112, values, 1);
        applied.Add(112, 1);
        return applied;
    }

    // always replaced: the falloff is always applied (with power 1 and gain 1 it is the game's own)
    inline bool Enabled() { return true; }

    inline ShaderPatcher::PatchedShader g_patch{"OmniLight", kOmniShaderHash, "omni_light", &Enabled, &PreDraw};
} // namespace ssa::OmniLightPatch
