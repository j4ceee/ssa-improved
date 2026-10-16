#pragma once

#include <iterator>
#include "magic_moment_patch.h"
#include "omni_light_patch.h"
#include "shader_patcher.h"
#include "sun_light_patch.h"
#include "water_patch.h"

// every pixel shader patch of the mod, and the entry points the D3D hooks use to talk to all of them
namespace ssa::ShaderPatches
{
    inline ShaderPatcher::PatchedShader* const kPatches[] = {
        &WaterPatch::g_patch,
        &OmniLightPatch::g_patch,
        &SunLightPatch::g_patch,
        &MagicMomentPatch::g_constAdd,
        &MagicMomentPatch::g_constMul,
        &MagicMomentPatch::g_texAdd,
        &MagicMomentPatch::g_texMul,
        &MagicMomentPatch::g_texPlain,
    };

    // called from ShaderTracker::OnPixelShaderCreated for every shader the game creates
    inline void OnShaderCreated(const void* shader, const uint64_t hash, const DWORD* fn, const size_t tokens)
    {
        for (ShaderPatcher::PatchedShader* patch : kPatches)
            patch->OnCreated(shader, hash, fn, tokens);
    }

    // maps one of our patched shaders back to the game's original object (see PatchedShader::ToGame)
    inline IDirect3DPixelShader9* ToGame(IDirect3DPixelShader9* shader)
    {
        for (const ShaderPatcher::PatchedShader* patch : kPatches)
            shader = patch->ToGame(shader);
        return shader;
    }

    // called from hook_SetPixelShader with the shader the game wants to bind; returns the shader to bind instead
    inline IDirect3DPixelShader9* Resolve(IDirect3DDevice9* dev, IDirect3DPixelShader9* shader)
    {
        for (ShaderPatcher::PatchedShader* patch : kPatches)
        {
            IDirect3DPixelShader9* resolved = patch->Resolve(dev, shader);
            if (resolved != shader) return resolved;
        }
        return shader;
    }

    // called right before every draw call with the pixel shader that is currently set; a shader belongs to at most one patch
    inline ShaderTweaks::Restore PreDraw(IDirect3DDevice9* dev, const void* currentPS)
    {
        for (const ShaderPatcher::PatchedShader* patch : kPatches)
            if (ShaderTweaks::Restore applied = patch->PreDraw(dev, currentPS); applied.n)
                return applied;
        return {};
    }
} // namespace ssa::ShaderPatches
