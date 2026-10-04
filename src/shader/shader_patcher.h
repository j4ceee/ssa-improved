#pragma once

#include <Windows.h>
#include <d3d9.h>

#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "compiled_shaders.h"
#include "log.h"
#include "shader_tweaks.h"

// Replacing pixel shaders of the game with the mod's own HLSL versions, found by the hash of their bytecode.
//
// The mod's shaders are written in HLSL and compiled when the mod is built; bytecode is part of the DLL.
// When the game creates a shader we only keep a copy of its bytecode if its hash matches a patch (it is used to check the replacement, see ConstantsMatch).
// First time the shader is bound a pixel shader is created from the compiled replacement and bound instead of the game's one.
// Everything else (tracking, tweaks, hiding) keeps seeing the game's original shader object.
// If anything goes wrong the game's shader is used and the reason is logged.
// The replacement is bound where the game's shader was, so it has to match its interface: same input / ... at the register the game sets it at.
// Before the replacement is used, every one of them is checked against the game's constant table, so a mismatch
// fails the patch instead of rendering rubbish. Constants the mod sets itself in PreDraw are the exception, they must be named ssa_*.
namespace ssa::ShaderPatcher
{
    using CreatePS_t = HRESULT(WINAPI*)(IDirect3DDevice9*, const DWORD*, IDirect3DPixelShader9**);

    // the ORIGINAL CreatePixelShader (bypasses our own hook), set once the device hooks are installed
    inline CreatePS_t g_createPS = nullptr;

    // -------------------------------------------------------------------------
    // helpers for writing patches
    // -------------------------------------------------------------------------

    // general 4x4 inverse (Gauss-Jordan with partial pivoting, in double precision); false if singular
    inline bool Invert4x4(const float in[16], float out[16])
    {
        double a[4][8];
        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c)
            {
                a[r][c] = in[r * 4 + c];
                a[r][c + 4] = (r == c) ? 1.0 : 0.0;
            }

        for (int col = 0; col < 4; ++col)
        {
            int pivot = col;
            for (int r = col + 1; r < 4; ++r)
                if (std::fabs(a[r][col]) > std::fabs(a[pivot][col])) pivot = r;
            if (std::fabs(a[pivot][col]) < 1e-12) return false;
            if (pivot != col)
                for (int c = 0; c < 8; ++c) std::swap(a[pivot][c], a[col][c]);

            const double inv = 1.0 / a[col][col];
            for (int c = 0; c < 8; ++c) a[col][c] *= inv;

            for (int r = 0; r < 4; ++r)
            {
                if (r == col) continue;
                const double f = a[r][col];
                if (f == 0.0) continue;
                for (int c = 0; c < 8; ++c) a[r][c] -= f * a[col][c];
            }
        }

        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c)
                out[r * 4 + c] = static_cast<float>(a[r][c + 4]);
        return true;
    }

    // For shaders of the deferred pipeline whose c0..c3 are mScreenToWorld (screen position + depth -> world position):
    // uploads the inverse (world -> screen) to c<firstRegister>..c<firstRegister + 3>, so the patch can project points back
    // onto the screen. Adds the registers to `applied`, PostDraw puts the game's values back.
    // The inverse is only recomputed when the matrix changes. If there is no usable matrix a matrix is uploaded that puts every
    // point behind the camera (w = -1), which patches treat as "nothing to find here".
    inline void UploadWorldToScreen(IDirect3DDevice9* dev, const UINT firstRegister, ShaderTweaks::Restore& applied)
    {
        static float s_source[16];
        static float s_inverse[16];
        static bool s_cached = false;

        const float* source = &ShaderTweaks::g_shadow[0];
        if (!s_cached || std::memcmp(source, s_source, sizeof(s_source)) != 0)
        {
            std::memcpy(s_source, source, sizeof(s_source));
            if (!Invert4x4(source, s_inverse))
            {
                std::memset(s_inverse, 0, sizeof(s_inverse));
                s_inverse[15] = -1.0f;
            }
            s_cached = true;
        }

        ShaderTweaks::g_setPSConstF(dev, firstRegister, s_inverse, 4);
        applied.Add(firstRegister, 4);
    }

    namespace detail
    {
        // CreatePixelShader of the game's device, protected against exceptions (drivers sometimes compile in there): a crash is reported
        // instead of taking the game down. Plain data only, __try cannot be used in a function that has objects with destructors.
        inline HRESULT CreateShaderSafe(const CreatePS_t create, IDirect3DDevice9* dev, const DWORD* code, IDirect3DPixelShader9** shader, DWORD& exceptionCode)
        {
            exceptionCode = 0;
            __try
            {
                return create(dev, code, shader);
            }
            __except (exceptionCode = GetExceptionCode(), EXCEPTION_EXECUTE_HANDLER)
            {
                return E_FAIL;
            }
        }
    } // namespace detail

    // -------------------------------------------------------------------------
    // PatchedShader
    // -------------------------------------------------------------------------
    class PatchedShader
    {
    public:
        // whether the patch is switched on right now (a settings value); nothing is loaded while it is off
        using EnabledFn = bool (*)();

        // sets the constants the patch needs right before a draw call that uses the patched shader; returns the registers it
        // changed (the hooks put the game's own values back from ShaderTweaks' shadow copy after the draw call)
        using PreDrawFn = ShaderTweaks::Restore (*)(IDirect3DDevice9* dev);

        // `name` is used for log messages, `compiledShader` is the name of the .hlsl file (without the extension) with the replacement
        PatchedShader(const char* name, const uint64_t hash, const char* compiledShader, const EnabledFn enabled,
                      const PreDrawFn preDraw = nullptr)
            : name_(name), hash_(hash), compiledShader_(compiledShader), enabled_(enabled), preDraw_(preDraw)
        {
        }

        PatchedShader(const PatchedShader&) = delete;
        PatchedShader& operator=(const PatchedShader&) = delete;

        // call from ShaderTracker::OnPixelShaderCreated for every shader the game creates
        void OnCreated(const void* shader, const uint64_t hash, const DWORD* fn, const size_t tokens)
        {
            if (hash != hash_)
            {
                // an old object of this shader at this address was replaced by something else
                if (original_.load() == shader) original_ = nullptr;
                return;
            }
            if (!fn || !tokens) return;

            std::lock_guard<std::mutex> lk(mutex_);
            code_.assign(fn, fn + tokens);
            original_ = shader;
        }

        // the game's shader object this patch is for (null until the game created it)
        [[nodiscard]] bool IsOriginal(const void* shader) const
        {
            return shader && shader == original_.load(std::memory_order_relaxed);
        }

        // true once the patched shader exists
        [[nodiscard]] bool IsActive() const { return patched_.load(std::memory_order_relaxed) != nullptr; }

        // The game can hand our patched object back to SetPixelShader (GetPixelShader + re-bind). Everything that identifies
        // shaders by the game's object must see the original, so map it back.
        [[nodiscard]] IDirect3DPixelShader9* ToGame(IDirect3DPixelShader9* shader) const
        {
            const void* original = original_.load(std::memory_order_relaxed);
            if (shader && original && shader == patched_.load(std::memory_order_relaxed))
                return static_cast<IDirect3DPixelShader9*>(const_cast<void*>(original));
            return shader;
        }

        // call from hook_SetPixelShader with the shader the game wants to bind; returns the shader to bind instead.
        // Creates the patched shader on first use.
        IDirect3DPixelShader9* Resolve(IDirect3DDevice9* dev, IDirect3DPixelShader9* shader)
        {
            if (!IsOriginal(shader) || !enabled_()) return shader;

            if (IDirect3DPixelShader9* patched = patched_.load(std::memory_order_acquire))
                return patched;

            std::lock_guard<std::mutex> lk(mutex_);
            if (!failed_)
                Create_NoLock(dev);
            IDirect3DPixelShader9* patched = patched_.load(std::memory_order_relaxed);
            return patched ? patched : shader;
        }

        // call right before a draw call, with the pixel shader that is currently set (the game's original object).
        // Returns the constants that PostDraw has to put back; n == 0: nothing was changed.
        ShaderTweaks::Restore PreDraw(IDirect3DDevice9* dev, const void* currentPS) const
        {
            if (!preDraw_ || !IsOriginal(currentPS) || !IsActive() || !enabled_() || !ShaderTweaks::g_setPSConstF)
                return {};
            return preDraw_(dev);
        }

    private:
        bool Fail_NoLock(const std::string& why)
        {
            failed_ = true;
            Log("[%s] patch failed, using the original shader: %s", name_, why.c_str());
            return false;
        }

        // true if every constant and sampler the compiled shader declares is at the register the game's shader has it at
        bool ConstantsMatch(const DWORD* compiled, const size_t tokens, std::string& mismatch) const
        {
            bool ok = true;
            ShaderTweaks::ForEachConstant(compiled, tokens, [&](const char* name, const int index)
            {
                if (std::strncmp(name, "ssa_", 4) == 0) return; // the mod's own, set in PreDraw

                const int expected = ShaderTweaks::FindConstantRegister(code_.data(), code_.size(), name);
                if (expected == index) return;

                ok = false;
                char buf[192];
                if (expected < 0)
                    snprintf(buf, sizeof(buf), "%s (register %d) is not in the game's shader; ", name, index);
                else
                    snprintf(buf, sizeof(buf), "%s is at register %d, the game's shader has it at %d; ", name, index,
                             expected);
                mismatch += buf;
            });
            return ok;
        }

        // check the compiled replacement against the game's shader, create the pixel shader
        bool Create_NoLock(IDirect3DDevice9* dev)
        {
            if (!g_createPS || code_.empty())
                return Fail_NoLock("device hooks or shader bytecode not ready");

            const CompiledShaders::Shader* compiled = CompiledShaders::Find(compiledShader_);
            if (!compiled)
                return Fail_NoLock(std::string("no compiled shader named ") + compiledShader_);

            const DWORD* code = reinterpret_cast<const DWORD*>(compiled->data);
            std::string mismatch;
            if (!ConstantsMatch(code, compiled->size / sizeof(DWORD), mismatch))
                return Fail_NoLock("its constants do not match the game's shader: " + mismatch);

            // create (original function, so our own CreatePixelShader hook does not treat it as a game shader)
            IDirect3DPixelShader9* shader = nullptr;
            DWORD exception = 0;
            const HRESULT created = detail::CreateShaderSafe(g_createPS, dev, code, &shader, exception);
            if (exception)
            {
                char buf[96];
                snprintf(buf, sizeof(buf), "CreatePixelShader crashed (exception 0x%08lX)", exception);
                return Fail_NoLock(buf);
            }
            if (FAILED(created) || !shader)
                return Fail_NoLock("CreatePixelShader rejected the compiled shader");

            patched_.store(shader, std::memory_order_release);
            Log("[%s] patched shader active", name_);
            return true;
        }

        const char* name_;
        uint64_t hash_;
        const char* compiledShader_;
        EnabledFn enabled_;
        PreDrawFn preDraw_;

        std::atomic<const void*> original_{nullptr}; // the game's shader object
        std::atomic<IDirect3DPixelShader9*> patched_{nullptr}; // our copy, null until created (written under mutex_)
        std::mutex mutex_; // guards the creation and code_
        std::vector<DWORD> code_; // the game's bytecode
        bool failed_ = false;
    };
} // namespace ssa::ShaderPatcher
