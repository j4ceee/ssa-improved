#pragma once

#include <Windows.h>
#include <d3d9.h>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <unordered_map>
#include "config.h"

// Live graphics sliders without replacing any shader.
// A tweak scales one component of a pixel shader constant right before a draw call and puts the game's own value back
// right after it. The shaders are found by the NAME of the constant in their embedded constant table (CTAB, read when
// the game creates the shader), so no shader hashes are needed and every variant that declares the constant is covered:
// - MaxBlur - .x - how strongly the depth of field blur is blended in
// - FogColor - .w - maximum fog amount of the material shaders
namespace ssa::ShaderTweaks
{
    struct Tweak
    {
        const char* constant; // name in the shader's constant table, as listed under "Parameters" in the dump
        uint8_t component;
        float (*factor)(); // current multiplier, 1.0 = leave the game's value alone
    };

    inline const Tweak kTweaks[] = {
        {"MaxBlur", 0, [] { return g_config.dofStrength; }},
        {"FogColor", 3, [] { return g_config.fogStrength; }},
    };
    inline constexpr size_t kTweakCount = std::size(kTweaks);
    static_assert(kTweakCount <= 4, "the registers of one shader are packed into 4 bytes");

    // ---------------------------------------------------------------------------------------------------------
    // state
    // ---------------------------------------------------------------------------------------------------------

    // ORIGINAL SetPixelShaderConstantF (bypasses our own hook), set once the device hooks are installed
    using SetPSConstF_t = HRESULT(WINAPI*)(IDirect3DDevice9*, UINT, const float*, UINT);
    inline SetPSConstF_t g_setPSConstF = nullptr;

    // shadow copy of the game's pixel shader constants (what the game last sent, per register)
    inline constexpr UINT kMaxRegs = 224;
    inline float g_shadow[kMaxRegs * 4] = {};

    // registers a shader keeps its tweakable constants in: one byte per kTweaks row, kNoReg = not declared
    inline constexpr uint32_t kNoReg = 0xFF;
    inline constexpr uint32_t kNoRegs = 0xFFFFFFFFu;

    inline std::mutex g_regsMutex;
    inline std::unordered_map<const void*, uint32_t> g_shaderRegs;
    // shader object -> packed registers (only shaders with a match)
    inline std::atomic<uint32_t> g_curRegs{kNoRegs}; // packed registers of the shader that is set right now

    // called from hook_SetPixelShaderConstantF with the data that is actually forwarded to D3D
    inline void NoteConstants(UINT start, const float* data, UINT count)
    {
        if (!data || start >= kMaxRegs) return;
        if (count > kMaxRegs - start) count = kMaxRegs - start;
        std::memcpy(&g_shadow[start * 4], data, count * 4 * sizeof(float));
    }

    // ---------------------------------------------------------------------------------------------------------
    // constant table lookup
    // ---------------------------------------------------------------------------------------------------------

    // Calls f(name, register) for every constant (and sampler) in the shader's constant table, the 'CTAB' comment the HLSL compiler
    // embeds. `tokens` is the number of DWORDs in the shader, END token included.
    template <class F>
    inline void ForEachConstant(const DWORD* fn, size_t tokens, F&& f)
    {
        struct Info
        {
            DWORD name;
            WORD set, index, count, reserved;
            DWORD type, def;
        }; // D3DXSHADER_CONSTANTINFO

        for (size_t n = 1; n < tokens;)
        {
            const DWORD t = fn[n];
            const DWORD op = t & 0xFFFF;
            if (op == 0xFFFF) return; // END
            if (op != 0xFFFE)
            {
                n += 1 + ((t >> 24) & 0x0F);
                continue;
            } // regular instruction

            const size_t len = (t >> 16) & 0x7FFF; // comment length in DWORDs
            if (len >= 8 && n + 1 + len <= tokens && fn[n + 1] == 0x42415443) // 'CTAB' + 28 byte header
            {
                const BYTE* base = reinterpret_cast<const BYTE*>(&fn[n + 2]);
                const size_t size = (len - 1) * sizeof(DWORD);
                DWORD count, infoOffset;
                std::memcpy(&count, base + 12, sizeof(count));
                std::memcpy(&infoOffset, base + 16, sizeof(infoOffset));

                if (infoOffset > size || count > (size - infoOffset) / sizeof(Info)) return;

                for (DWORD i = 0; i < count; ++i)
                {
                    Info info;
                    std::memcpy(&info, base + infoOffset + i * sizeof(Info), sizeof(Info));
                    if (info.name < size)
                        f(reinterpret_cast<const char*>(base) + info.name, static_cast<int>(info.index));
                }
                return;
            }
            n += 1 + len;
        }
    }

    // register index of the named constant in the shader's constant table (the 'CTAB' comment the HLSL compiler embeds), or -1. `tokens` is the number of DWORDs in the shader, END token included.
    inline int FindConstantRegister(const DWORD* fn, size_t tokens, const char* wanted)
    {
        int found = -1;
        ForEachConstant(fn, tokens, [&](const char* name, int index)
        {
            if (found < 0 && std::strcmp(name, wanted) == 0) found = index;
        });
        return found;
    }

    // ---------------------------------------------------------------------------------------------------------
    // shader created / set
    // ---------------------------------------------------------------------------------------------------------

    // call from ShaderTracker::OnPixelShaderCreated (fn/tokens = the shader bytecode, tokens may be 0 if unparsed)
    inline void OnShaderCreated(const void* shader, const DWORD* fn, size_t tokens)
    {
        uint32_t packed = kNoRegs;
        bool any = false;
        if (fn && tokens)
        {
            for (size_t i = 0; i < kTweakCount; ++i)
            {
                const int reg = FindConstantRegister(fn, tokens, kTweaks[i].constant);
                if (reg < 0 || static_cast<UINT>(reg) >= kMaxRegs) continue;

                packed &= ~(0xFFu << (8 * i));
                packed |= static_cast<uint32_t>(reg) << (8 * i);
                any = true;
            }
        }

        std::lock_guard<std::mutex> lk(g_regsMutex);
        if (any) g_shaderRegs[shader] = packed;
        else g_shaderRegs.erase(shader); // an old shader object at this address was replaced by something else
    }

    // called from hook_SetPixelShader
    inline void OnSetPixelShader(const void* shader)
    {
        uint32_t packed = kNoRegs;
        if (shader)
        {
            bool active = false;
            for (const Tweak& t : kTweaks)
                if (t.factor() != 1.0f)
                {
                    active = true;
                    break;
                }

            if (active)
            {
                std::lock_guard<std::mutex> lk(g_regsMutex);
                if (auto it = g_shaderRegs.find(shader); it != g_shaderRegs.end())
                    packed = it->second;
            }
        }
        g_curRegs.store(packed, std::memory_order_relaxed);
    }

    // ---------------------------------------------------------------------------------------------------------
    // per-draw hooks
    // ---------------------------------------------------------------------------------------------------------

    // constant registers a draw call changed. PostDraw puts the game's values back from the shadow copy, so it
    // needs no knowledge of which tweak (or the water patch) made the change. n == 0: nothing to restore.
    struct Restore
    {
        struct Range
        {
            UINT start, count;
        };

        Range ranges[4] = {};
        uint8_t n = 0;

        void Add(UINT start, UINT count)
        {
            if (n < 4) ranges[n++] = {start, count};
        }
    };

    // called right before a draw call
    inline Restore PreDraw(IDirect3DDevice9* dev)
    {
        const uint32_t packed = g_curRegs.load(std::memory_order_relaxed);
        if (packed == kNoRegs || !g_setPSConstF) return {};

        Restore applied;
        for (size_t i = 0; i < kTweakCount; ++i)
        {
            const uint32_t reg = (packed >> (8 * i)) & 0xFF;
            if (reg == kNoReg) continue;

            const Tweak& t = kTweaks[i];
            const float f = t.factor();
            if (f == 1.0f) continue;

            float v[4];
            std::memcpy(v, &g_shadow[reg * 4], sizeof(v));
            if (v[t.component] == 0.0f) continue; // never saw the game's value (or it is 0), don't guess

            v[t.component] *= f;
            g_setPSConstF(dev, reg, v, 1);
            applied.Add(reg, 1);
        }
        return applied;
    }

    // called right after the draw call: puts the game's own values back
    inline void PostDraw(IDirect3DDevice9* dev, const Restore& applied)
    {
        for (uint8_t i = 0; i < applied.n; ++i)
        {
            const Restore::Range& r = applied.ranges[i];
            g_setPSConstF(dev, r.start, &g_shadow[r.start * 4], r.count);
        }
    }
} // namespace ssa::ShaderTweaks
