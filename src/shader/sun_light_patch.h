#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include "config.h"
#include "log.h"
#include "shader_patcher.h"

// Replacement of the game's sunlight pixel shader (see shader_patcher.h for how a patch is bound): contact-hardening soft shadows
// (PCSS). The shader itself is hlsl/sun_light.hlsl.
namespace ssa::SunLightPatch
{
    // FNV-1a of the sunlight shader
    inline constexpr uint64_t kSunShaderHash = 0xF5601E802A03558Bull;

    // live tuning (Diagnostics > Lighting shader tuning); copy values you like into the defaults
    struct Tuning
    {
        float minScale = 0.05f; // filter radius where the caster touches the receiver, as a multiple of the game's radius
        float maxScale = 4.0f; // ... and as far from the caster as softDistance, again a multiple of the game's radius
        float softDistance = 40.0f; // distance between caster and receiver (world units) at which the maximum is reached
        float searchScale = 6.0f; // radius of the blocker search, a multiple of the game's radius (should reach maxScale * 1.9)
        float blockerBias = 0.02f; // a shadow map texel counts as a blocker if it is at least this much (world units) nearer to the sun
    };

    inline Tuning g_tuning;

    // Per draw: ssa_PcssA / ssa_PcssB (c31-c32) hold the tuning, converted to the shader's units from the game's own shadow map matrix
    // (c8-c11, world -> shadow map: position = x * c8 + y * c9 + z * c10 + c11, so the depth (z) of the map changes by
    // |(c8.z, c9.z, c10.z)| per world unit). The hooks put the game's own values back after the draw call.
    inline ShaderTweaks::Restore PreDraw(IDirect3DDevice9* dev)
    {
        const float* c = &ShaderTweaks::g_shadow[0];
        const auto row = [c](const int reg, const int comp) { return c[reg * 4 + comp]; };

        const float depthPerWorld = std::sqrt(row(8, 2) * row(8, 2) + row(9, 2) * row(9, 2) + row(10, 2) * row(10, 2));
        const float scale = depthPerWorld > 1e-6f ? depthPerWorld : 1.0f; // no matrix seen yet: meaningless but harmless values

        // one-time info for tuning
        static bool s_logged = false;
        if (!s_logged && depthPerWorld > 1e-6f)
        {
            s_logged = true;
            const float uvX = std::sqrt(row(8, 0) * row(8, 0) + row(9, 0) * row(9, 0) + row(10, 0) * row(10, 0));
            const float radius = row(19, 1);
            Log("[SunLight] shadow map 0: %.5f uv per world unit, %.5f depth per world unit, the game's filter radius is %.5f uv (%.3f world units)",
                uvX, depthPerWorld, radius, uvX > 1e-6f ? radius / uvX : 0.0f);
        }

        const Tuning& t = g_tuning;
        const float values[8] = {
            // ssa_PcssA: depth difference -> softness. Depth changes by `scale` per world unit, so a caster-receiver distance of
            // softDistance world units is a depth difference of scale * softDistance
            t.minScale, t.maxScale - t.minScale, 1.0f / (scale * std::max(t.softDistance, 0.01f)), t.searchScale,
            t.blockerBias * scale, 0.0f, 0.0f, 0.0f, // ssa_PcssB
        };
        ShaderTweaks::g_setPSConstF(dev, 31, values, 2);

        ShaderTweaks::Restore applied;
        applied.Add(31, 2);
        return applied;
    }

    inline bool Enabled() { return g_config.softSunShadows; }

    inline ShaderPatcher::PatchedShader g_patch{"SunLight", kSunShaderHash, "sun_light", &Enabled, &PreDraw};
} // namespace ssa::SunLightPatch
