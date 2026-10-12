#pragma once
#include <Windows.h>
#include <d3d9.h>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <emmintrin.h>

// Drops D3D9 state calls that would not change anything
// - game sets all of its state per draw call and relies on the driver to ignore repeats: about 80% of the SetRenderState / SetSamplerState / Set*ShaderConstant
//   calls in a frame send the value the device already has
// - on a D3DCREATE_MULTITHREADED device every one of those takes a lock and goes through the runtime
//
// The cache mirrors what was last sent to the device and filters repeats. It only holds when EVERY write goes through it:
// - all hooked Set* calls go through the Redundant* functions below
// - ShaderTweaks / ShaderPatches write pixel shader constants behind our back but always put the game's values back right after the draw call, so the
//   device holds the game's values whenever the game sends something
// - everything else that can change device state behind the hooks calls Invalidate()
// - calls from any thread other than the render thread (load screen) are forwarded and invalidate everything
// - Present invalidates as well, so a missed write could only ever be wrong for a single frame
// A mirror slot is valid while its stamp equals the current generation, so Invalidate() is a single increment.
namespace ssa::StateCache
{
    inline bool g_enabled = true;
    inline DWORD g_ownerThread = 0; // the render thread (the one that created the device)
    inline std::atomic<uint32_t> g_generation{1};

    inline void Invalidate() { g_generation.fetch_add(1, std::memory_order_relaxed); }

    enum Kind { RenderState, SamplerState, VsConstF, VsConstB, PsConstF, PsConstB, KindCount };

    struct Counter
    {
        uint32_t seen = 0;
        uint32_t filtered = 0;
    };

    inline Counter g_cur[KindCount];
    inline Counter g_last[KindCount]; // previous frame, for the diagnostics window

    inline const char* KindName(Kind k)
    {
        static constexpr const char* names[KindCount] = {
            "SetRenderState", "SetSamplerState", "SetVertexShaderConstantF", "SetVertexShaderConstantB",
            "SetPixelShaderConstantF", "SetPixelShaderConstantB",
        };
        return names[k];
    }

    namespace detail
    {
        struct Dword
        {
            uint32_t value = 0;
            uint32_t stamp = 0;
        };

        struct Vec4
        {
            float v[4] = {};
            uint32_t stamp = 0;
        };

        inline constexpr UINT kRenderStates = 256; // D3DRS_* go up to 209
        inline constexpr UINT kSamplerSlots = 20; // 16 pixel samplers + 4 vertex texture samplers
        inline constexpr UINT kSamplerTypes = 14; // D3DSAMP_* 1..13
        inline constexpr UINT kVsFloatRegs = 256;
        inline constexpr UINT kPsFloatRegs = 224;
        inline constexpr UINT kBoolRegs = 16;

        inline Dword g_renderStates[kRenderStates];
        inline Dword g_samplerStates[kSamplerSlots][kSamplerTypes];
        inline Vec4 g_vsF[kVsFloatRegs];
        inline Vec4 g_psF[kPsFloatRegs];
        inline Dword g_vsB[kBoolRegs];
        inline Dword g_psB[kBoolRegs];

        // false = forward the call as usual (cache off, or not the render thread)
        inline bool Begin(Kind kind)
        {
            if (!g_enabled)
                return false;
            if (GetCurrentThreadId() != g_ownerThread)
            {
                Invalidate(); // this call changes device state without the mirror knowing
                return false;
            }
            ++g_cur[kind].seen;
            return true;
        }

        inline bool SameDword(Kind kind, Dword& slot, uint32_t value)
        {
            const uint32_t gen = g_generation.load(std::memory_order_relaxed);
            if (slot.stamp == gen && slot.value == value)
            {
                ++g_cur[kind].filtered;
                return true;
            }
            slot.value = value;
            slot.stamp = gen;
            return false;
        }

        // bit-exact compare of two float4 (like memcmp) without a call per register: SameFloats runs for every constant the game sets
        inline bool SameVec4(const float* a, const float* b)
        {
            const __m128i x = _mm_loadu_si128(reinterpret_cast<const __m128i*>(a));
            const __m128i y = _mm_loadu_si128(reinterpret_cast<const __m128i*>(b));
            return _mm_movemask_epi8(_mm_cmpeq_epi8(x, y)) == 0xFFFF;
        }

        inline bool SameFloats(Kind kind, Vec4* bank, UINT bankSize, UINT start, const float* data, UINT count)
        {
            if (!data || start >= bankSize || count > bankSize - start)
            {
                Invalidate(); // a write the mirror cannot hold
                return false;
            }

            const uint32_t gen = g_generation.load(std::memory_order_relaxed);
            bool same = true;
            for (UINT i = 0; i < count && same; ++i)
            {
                const Vec4& r = bank[start + i];
                same = r.stamp == gen && SameVec4(r.v, data + i * 4);
            }
            if (same)
            {
                ++g_cur[kind].filtered;
                return true;
            }

            for (UINT i = 0; i < count; ++i)
            {
                Vec4& r = bank[start + i];
                std::memcpy(r.v, data + i * 4, sizeof(r.v));
                r.stamp = gen;
            }
            return false;
        }

        inline bool SameBools(Kind kind, Dword* bank, UINT start, const BOOL* data, UINT count)
        {
            if (!data || start >= kBoolRegs || count > kBoolRegs - start)
            {
                Invalidate();
                return false;
            }

            const uint32_t gen = g_generation.load(std::memory_order_relaxed);
            bool same = true;
            for (UINT i = 0; i < count && same; ++i)
                same = bank[start + i].stamp == gen && bank[start + i].value == static_cast<uint32_t>(data[i]);
            if (same)
            {
                ++g_cur[kind].filtered;
                return true;
            }

            for (UINT i = 0; i < count; ++i)
            {
                bank[start + i].value = static_cast<uint32_t>(data[i]);
                bank[start + i].stamp = gen;
            }
            return false;
        }
    } // namespace detail

    // -------------------------------------------------------------------------
    // each function returns true if the call can be dropped (the device already has this value).
    // When it returns false the mirror has been updated and the call must be forwarded.
    // If the forwarded call fails, call Invalidate().
    // -------------------------------------------------------------------------

    inline bool RedundantRenderState(D3DRENDERSTATETYPE state, DWORD value)
    {
        if (!detail::Begin(RenderState) || static_cast<UINT>(state) >= detail::kRenderStates)
            return false;
        return detail::SameDword(RenderState, detail::g_renderStates[state], value);
    }

    inline bool RedundantSamplerState(DWORD sampler, D3DSAMPLERSTATETYPE type, DWORD value)
    {
        if (!detail::Begin(SamplerState))
            return false;

        UINT slot;
        if (sampler < 16)
            slot = sampler;
        else if (sampler >= D3DVERTEXTEXTURESAMPLER0 && sampler < D3DVERTEXTEXTURESAMPLER0 + 4)
            slot = 16 + (sampler - D3DVERTEXTEXTURESAMPLER0);
        else
            return false; // displacement map sampler: not stored, so nothing in the mirror can go stale

        if (static_cast<UINT>(type) < 1 || static_cast<UINT>(type) >= detail::kSamplerTypes)
            return false;
        return detail::SameDword(SamplerState, detail::g_samplerStates[slot][type], value);
    }

    inline bool RedundantVsConstF(UINT start, const float* data, UINT count)
    {
        if (!detail::Begin(VsConstF))
            return false;
        return detail::SameFloats(VsConstF, detail::g_vsF, detail::kVsFloatRegs, start, data, count);
    }

    inline bool RedundantVsConstB(UINT start, const BOOL* data, UINT count)
    {
        if (!detail::Begin(VsConstB))
            return false;
        return detail::SameBools(VsConstB, detail::g_vsB, start, data, count);
    }

    inline bool RedundantPsConstF(UINT start, const float* data, UINT count)
    {
        if (!detail::Begin(PsConstF))
            return false;
        return detail::SameFloats(PsConstF, detail::g_psF, detail::kPsFloatRegs, start, data, count);
    }

    inline bool RedundantPsConstB(UINT start, const BOOL* data, UINT count)
    {
        if (!detail::Begin(PsConstB))
            return false;
        return detail::SameBools(PsConstB, detail::g_psB, start, data, count);
    }

    // mod wrote pixel shader constants behind the cache (ShaderTweaks / ShaderPatches around a draw call): forget those.
    // only the registers that were written: the rest of the mirror is still exact, and wiping it makes every constant of the next draw call go through to D3D again.
    inline void InvalidatePsConstF(UINT start, UINT count)
    {
        for (UINT i = start; i < start + count && i < detail::kPsFloatRegs; ++i)
            detail::g_psF[i].stamp = 0; // 0 never equals a generation
    }

    // every pixel shader constant (kept for callers that cannot tell which registers they touched)
    inline void InvalidatePsConstF()
    {
        InvalidatePsConstF(0, detail::kPsFloatRegs);
    }

    // called once per frame from Present
    inline void OnPresent()
    {
        for (int i = 0; i < KindCount; ++i)
        {
            g_last[i] = g_cur[i];
            g_cur[i] = Counter{};
        }
        Invalidate();
    }
} // namespace ssa::StateCache
