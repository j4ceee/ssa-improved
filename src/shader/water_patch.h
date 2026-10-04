#pragma once

#include <Windows.h>
#include <d3d9.h>
#include <cmath>
#include <cstdint>
#include <string>
#include "config.h"
#include "shader_patcher.h"

// Replacement of the game's water pixel shader (see shader_patcher.h for how a patch is bound). The shader itself, with all its tuning
// values, is hlsl/water.hlsl: screen-space reflections, a fix of the refraction, caustics and shoreline foam.
namespace ssa::WaterPatch
{
    // FNV-1a of the water shader's bytecode, as shown in the Pixel shaders tab
    inline constexpr uint64_t kWaterShaderHash = 0xB1972FA40FA666B8ull;

    // Seconds for the caustics and foam. All their animations use whole multiples of it, so wrapping after 1000 full turns of the
    // fastest one is not visible.
    inline float AnimationTime()
    {
        static const double kFrequency = []
        {
            LARGE_INTEGER f;
            QueryPerformanceFrequency(&f);
            return static_cast<double>(f.QuadPart);
        }();
        constexpr double kPi = 3.14159265358979323846;

        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        return static_cast<float>(std::fmod(static_cast<double>(now.QuadPart) / kFrequency, 2000.0 * kPi));
    }

    // sets the constants the water shader needs right before a draw call: ssa_mWorldToScreen (c100-c103) and ssa_Time (c113, the
    // time changes every frame so it cannot be part of the shader). The shadow copy holds whatever the game itself last put there,
    // another shader may rely on that, so the hooks put it back after the draw call.
    inline ShaderTweaks::Restore PreDraw(IDirect3DDevice9* dev)
    {
        ShaderTweaks::Restore applied;

        // c0 - c3 of the water shader = mScreenToWorld; the shader needs the opposite direction (world -> screen).
        // Without a usable matrix every ray sample counts as "behind the camera" and the cube map is used instead.
        ShaderPatcher::UploadWorldToScreen(dev, 100, applied);

        const float time[4] = {AnimationTime(), 0.0f, 0.0f, 0.0f};
        ShaderTweaks::g_setPSConstF(dev, 113, time, 1);
        applied.Add(113, 1);
        return applied;
    }

    inline bool Enabled() { return g_config.waterReflections; }

    inline ShaderPatcher::PatchedShader g_patch{"Water", kWaterShaderHash, "water", &Enabled, &PreDraw};
} // namespace ssa::WaterPatch
