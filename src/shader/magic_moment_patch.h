#pragma once

#include <cstdint>
#include "config.h"
#include "shader_patcher.h"

// Replacement of the game's character pixel shaders for the magic moment (see shader_patcher.h for how a patch is bound).
// The game draws the Skylander with "PIP scene" variants of its character material, one shader per combination of specular
// and sphere map handling, and every one of them lights the character with one fixed direction.
// The patches swap that for a light from below, like the Wii U and Giants versions have. The shaders are hlsl/magic_moment_*.hlsl,
// they share hlsl/magic_moment.hlsli, which also holds the tuning values.
namespace ssa::MagicMomentPatch
{
    // switched at runtime by the setting: while it is off the game's own shaders are bound, nothing is loaded
    inline bool Enabled() { return g_config.magicMomentLighting; }

    // specular from constants + a specular map, sphere map added to the colour
    inline ShaderPatcher::PatchedShader g_constAdd{"MagicMoment (const, add)", 0xFAF245AA70A45977ull, "magic_moment_const_add", &Enabled};
    // specular from constants + a specular map, sphere map multiplied with the colour
    inline ShaderPatcher::PatchedShader g_constMul{"MagicMoment (const, mul)", 0x08CB1D44EE383161ull, "magic_moment_const_mul", &Enabled};
    // specular colour and power from textures, sphere map added
    inline ShaderPatcher::PatchedShader g_texAdd{"MagicMoment (tex, add)", 0xD42086C024107981ull, "magic_moment_tex_add", &Enabled};
    // specular colour and power from textures, sphere map multiplied (metal)
    inline ShaderPatcher::PatchedShader g_texMul{"MagicMoment (tex, mul)", 0x30FF1097659DD78Bull, "magic_moment_tex_mul", &Enabled};
    // specular colour and power from textures, no sphere map
    inline ShaderPatcher::PatchedShader g_texPlain{"MagicMoment (tex, plain)", 0x28469059F169A843ull, "magic_moment_tex_plain", &Enabled};
} // namespace ssa::MagicMomentPatch
