#pragma once
#include <Windows.h>
#include <d3d9.h>
#include <array>
#include <memory>
#include "config.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#include "log.h"
#include "MinHook.h"
#include "window_hooks.h"
#include "monitor.h"
#include "texture_mods.h"
#include "state_cache.h"
#include "async_readback.h"
#include "patches.h"
#include "render_targets.h"
#include "shader/shader_tracker.h"
#include "shader/shader_tweaks.h"
#include "shader/shader_patches.h"
#include "shader/ui_hider.h"
#include "game/custom/cinema_skip.h"
#include "securom/constant_lookup.h"
#include "securom/lua_tables.h"
#include "securom/render_list.h"
#include "game/custom/difficulty.h"
#include "game/custom/free_cam.h"
#include "game/custom/items.h"
#include "game/custom/jump.h"
#include "game/custom/load_screen.h"
#include "game/custom/music.h"
#include "game/custom/skylanderSettings.h"
#include "game/custom/attack_lights.h"
#include "game/custom/skylanderMovement.h"

namespace ssa::D3D9Hooks
{
    // -------------------------------------------------------------------------
    // function pointer typedefs
    // -------------------------------------------------------------------------
    using Direct3DCreate9_t             = IDirect3D9*(WINAPI*)(UINT);
    using CreateDevice_t                = HRESULT(WINAPI*)(IDirect3D9*, UINT, D3DDEVTYPE, HWND, DWORD, D3DPRESENT_PARAMETERS*, IDirect3DDevice9**);
    using Reset_t                       = HRESULT(WINAPI*)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);
    using EndScene_t                    = HRESULT(WINAPI*)(IDirect3DDevice9*);
    using Present_t                     = HRESULT(WINAPI*)(IDirect3DDevice9*, const RECT*, const RECT*, HWND, const RGNDATA*);
    using SetSamplerState_t             = HRESULT(WINAPI*)(IDirect3DDevice9*, DWORD, D3DSAMPLERSTATETYPE, DWORD);
    using CreateTexture_t               = HRESULT(WINAPI*)(IDirect3DDevice9*, UINT, UINT, UINT, DWORD, D3DFORMAT, D3DPOOL, IDirect3DTexture9**, HANDLE*);
    using CreateDepthStencilSurface_t   = HRESULT(WINAPI*)(IDirect3DDevice9*, UINT, UINT, D3DFORMAT, D3DMULTISAMPLE_TYPE, DWORD, BOOL, IDirect3DSurface9**, HANDLE*);
    using SetViewport_t                 = HRESULT(WINAPI*)(IDirect3DDevice9*, const D3DVIEWPORT9*);
    using SetScissorRect_t              = HRESULT(WINAPI*)(IDirect3DDevice9*, const RECT*);
    using SetPixelShaderConstantF_t     = HRESULT(WINAPI*)(IDirect3DDevice9*, UINT, const float*, UINT);
    using SetTexture_t                  = HRESULT(WINAPI*)(IDirect3DDevice9*, DWORD, IDirect3DBaseTexture9*);
    using SetRenderTarget_t             = HRESULT(WINAPI*)(IDirect3DDevice9*, DWORD, IDirect3DSurface9*);
    using CreatePixelShader_t           = HRESULT(WINAPI*)(IDirect3DDevice9*, const DWORD*, IDirect3DPixelShader9**);
    using SetPixelShader_t              = HRESULT(WINAPI*)(IDirect3DDevice9*, IDirect3DPixelShader9*);
    using DrawPrimitive_t               = HRESULT(WINAPI*)(IDirect3DDevice9*, D3DPRIMITIVETYPE, UINT, UINT);
    using DrawIndexedPrimitive_t        = HRESULT(WINAPI*)(IDirect3DDevice9*, D3DPRIMITIVETYPE, INT, UINT, UINT, UINT, UINT);
    using DrawPrimitiveUP_t             = HRESULT(WINAPI*)(IDirect3DDevice9*, D3DPRIMITIVETYPE, UINT, const void*, UINT);
    using DrawIndexedPrimitiveUP_t      = HRESULT(WINAPI*)(IDirect3DDevice9*, D3DPRIMITIVETYPE, UINT, UINT, UINT, const void*, D3DFORMAT, const void*, UINT);
    using SetRenderState_t              = HRESULT(WINAPI*)(IDirect3DDevice9*, D3DRENDERSTATETYPE, DWORD);
    using SetVertexShaderConstantF_t    = HRESULT(WINAPI*)(IDirect3DDevice9*, UINT, const float*, UINT);
    using SetVertexShaderConstantB_t    = HRESULT(WINAPI*)(IDirect3DDevice9*, UINT, const BOOL*, UINT);
    using SetPixelShaderConstantB_t     = HRESULT(WINAPI*)(IDirect3DDevice9*, UINT, const BOOL*, UINT);

    inline Direct3DCreate9_t            orig_Direct3DCreate9            = nullptr;
    inline CreateDevice_t               orig_CreateDevice               = nullptr;
    inline Reset_t                      orig_Reset                      = nullptr;
    inline EndScene_t                   orig_EndScene                   = nullptr;
    inline Present_t                    orig_Present                    = nullptr;
    inline SetSamplerState_t            orig_SetSamplerState            = nullptr;
    inline CreateTexture_t              orig_CreateTexture              = nullptr;
    inline CreateDepthStencilSurface_t  orig_CreateDepthStencilSurface  = nullptr;
    inline SetViewport_t                orig_SetViewport                = nullptr;
    inline SetScissorRect_t             orig_SetScissorRect             = nullptr;
    inline SetPixelShaderConstantF_t    orig_SetPixelShaderConstantF    = nullptr;
    inline SetTexture_t                 orig_SetTexture                 = nullptr;
    inline SetRenderTarget_t            orig_SetRenderTarget            = nullptr;
    inline CreatePixelShader_t          orig_CreatePixelShader          = nullptr;
    inline SetPixelShader_t             orig_SetPixelShader             = nullptr;
    inline DrawPrimitive_t              orig_DrawPrimitive              = nullptr;
    inline DrawIndexedPrimitive_t       orig_DrawIndexedPrimitive       = nullptr;
    inline DrawPrimitiveUP_t            orig_DrawPrimitiveUP            = nullptr;
    inline DrawIndexedPrimitiveUP_t     orig_DrawIndexedPrimitiveUP     = nullptr;
    inline SetRenderState_t             orig_SetRenderState             = nullptr;
    inline SetVertexShaderConstantF_t   orig_SetVertexShaderConstantF   = nullptr;
    inline SetVertexShaderConstantB_t   orig_SetVertexShaderConstantB   = nullptr;
    inline SetPixelShaderConstantB_t    orig_SetPixelShaderConstantB    = nullptr;

    // -------------------------------------------------------------------------
    // state
    // -------------------------------------------------------------------------
    inline int g_displayRefreshHz = 0;
    inline UINT g_bbWidth = 0;
    inline UINT g_bbHeight = 0;
    inline IDirect3DDevice9* g_d3dDevice = nullptr;
    inline DWORD g_mainThreadId = 0;
    inline HWND g_hFocusWindow = nullptr; // focus window the game created the device with (ImGui's window)

    // internal resolution the game uses for its 3D scene pipeline
    static constexpr UINT k_internalW = 1120;
    static constexpr UINT k_internalH = 704;

    // -------------------------------------------------------------------------
    // dimension scaling table
    // maps known internal RT/viewport sizes to their scaled equivalents
    // -------------------------------------------------------------------------

    inline UINT SsSq(UINT base) { return (UINT)roundf(base * g_config.ssMultiplier); }
    inline UINT SsW() { return (UINT)roundf(g_bbWidth  * g_config.ssMultiplier); }
    inline UINT SsH() { return (UINT)roundf(g_bbHeight * g_config.ssMultiplier); }
    inline UINT ShadowRes() { return static_cast<UINT>(g_config.shadowRes); }

    struct ScaleTier
    {
        UINT srcW, srcH; // size the game asks for
        UINT dstW, dstH; // size we use instead
        float invSrcW, invSrcH; // 1/src, used to recognise texel-size shader constants
    };

    inline constexpr size_t kScaleTierCount = 8;

    // Rebuilt only when something feeding it changes. Every viewport, scissor and pixel shader constant upload goes through here, so it must not be recomputed per call.
    inline const std::array<ScaleTier, kScaleTierCount>& ScaleTiers()
    {
        static std::array<ScaleTier, kScaleTierCount> tiers;
        static UINT keyW = 0, keyH = 0, keyShadow = 0;
        static float keyScale = -1.f;

        if (keyW != g_bbWidth || keyH != g_bbHeight || keyScale != g_config.ssMultiplier || keyShadow != ShadowRes())
        {
            keyW = g_bbWidth;
            keyH = g_bbHeight;
            keyScale = g_config.ssMultiplier;
            keyShadow = ShadowRes();

            const auto make = [](UINT sw, UINT sh, UINT dw, UINT dh)
            {
                return ScaleTier{sw, sh, dw, dh, 1.f / sw, 1.f / sh};
            };
            tiers = {
                {
                    make(k_internalW,       k_internalH,        SsW(),          SsH()           ), // main scene color RT
                    make(k_internalW / 2,   k_internalH / 2,    SsW() / 2,      SsH() / 2       ), // half-res post-fx
                    make(k_internalW / 4,   k_internalH / 4,    SsW() / 4,      SsH() / 4       ), // quarter-res downsample + SSAO
                    make(288,               176,                SsW() / 4,      SsH() / 4       ), // bloom quarter ping-pong
                    make(144,               96,                 SsW() / 8,      SsH() / 8       ), // bloom eighth blur chain
                    make(512,               512,                SsSq(512), SsSq(512)  ), // mirror color RT
                    make(256,               256,                SsSq(256), SsSq(256)  ), // mirror blur/VSM target
                    make(1024,              1024,               ShadowRes(),    ShadowRes()     ), // shadow maps
                }
            };
        }
        return tiers;
    }

    inline bool TryScaleDimensions(UINT& w, UINT& h)
    {
        for (const auto& t : ScaleTiers()) {
            if (w == t.srcW && h == t.srcH) {
                w = t.dstW;
                h = t.dstH;
                return true;
            }
        }
        return false;
    }

    // -------------------------------------------------------------------------
    // Helpers
    // -------------------------------------------------------------------------

    inline void ApplyPresentParams(D3DPRESENT_PARAMETERS* pp)
    {
        if (!pp) return;

        // fullscreen is not supported, always run windowed
        pp->Windowed = TRUE;
        pp->FullScreen_RefreshRateInHz = 0;

        const bool hasCustomRes = g_config.resolutionW > 0 && g_config.resolutionH > 0;

        if (g_config.borderless) {
            // always fill the target monitor; fall back to its native res if no custom res set
            const Monitor::Target& mon = Monitor::Get();
            pp->BackBufferWidth  = hasCustomRes ? g_config.resolutionW : mon.width();
            pp->BackBufferHeight = hasCustomRes ? g_config.resolutionH : mon.height();
        } else if (hasCustomRes) {
            // plain windowed with custom res: apply it
            pp->BackBufferWidth  = g_config.resolutionW;
            pp->BackBufferHeight = g_config.resolutionH;
        }
        // else: leave backbuffer dimensions untouched - game decides

        // save backbuffer dimensions to apply to internal rendering
        g_bbWidth  = pp->BackBufferWidth;
        g_bbHeight = pp->BackBufferHeight;

        pp->PresentationInterval = g_config.vsync ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;

        Log("[D3D9] Present params: %dx%d windowed=%d vsync=%d",
            pp->BackBufferWidth, pp->BackBufferHeight, pp->Windowed, g_config.vsync);
    }

    // -------------------------------------------------------------------------
    // patch a vec4 of texel-size shader constants from the internal resolution to the actual backbuffer resolution
    // game uses two patterns: (2/W, 2/H, 1/W, 1/H) & (1/W, 1/H, 0.5/W, 0.5/H)
    // -------------------------------------------------------------------------
    inline bool FixTexelSizeVec4(float* v)
    {
        // texel sizes are small positive numbers (the largest is 2/96); matrices, colours etc. fail this immediately
        constexpr float kMaxTexel = 0.025f;
        for (int i = 0; i < 4; ++i)
            if (!(v[i] > 0.f && v[i] < kMaxTexel)) return false;

        const float eps = 0.01f;
        auto fNear = [&](float a, float b) { return fabsf(a - b) < fabsf(b) * eps; };

        for (const auto& t : ScaleTiers())
        {
            const float iW = t.invSrcW;
            const float iH = t.invSrcH;

            // pattern: (2/W, 2/H, 1/W, 1/H)
            if (fNear(v[0], 2 * iW) && fNear(v[1], 2 * iH) && fNear(v[2], iW) && fNear(v[3], iH))
            {
                float nW = 1.f / t.dstW, nH = 1.f / t.dstH;
                v[0] = 2 * nW;
                v[1] = 2 * nH;
                v[2] = nW;
                v[3] = nH;
                return true;
            }

            // pattern: (1/W, 1/H, 0.5/W, 0.5/H)
            if (fNear(v[0], iW) && fNear(v[1], iH) && fNear(v[2], .5f * iW) && fNear(v[3], .5f * iH))
            {
                float nW = 1.f / t.dstW, nH = 1.f / t.dstH;
                v[0] = nW;
                v[1] = nH;
                v[2] = .5f * nW;
                v[3] = .5f * nH;
                return true;
            }
        }

        return false;
    }

    // result of a state call that was forwarded to D3D: a failure means the device may not have taken the value the state cache assumed
    inline HRESULT Forwarded(HRESULT hr)
    {
        if (FAILED(hr))
            StateCache::Invalidate();
        return hr;
    }

    // -------------------------------------------------------------------------
    // Hook: SetPixelShaderConstantF - patches texel-size uniforms for nativeRes
    // -------------------------------------------------------------------------
    inline HRESULT WINAPI hook_SetPixelShaderConstantF(IDirect3DDevice9* pDevice, UINT StartRegister, const float* pData, UINT Vector4fCount)
    {
        if (!pData)
            return orig_SetPixelShaderConstantF(pDevice, StartRegister, pData, Vector4fCount);

        const float* forwarded = pData;
        float stackBuf[32 * 4]; // small batches (typical) stay on the stack
        std::unique_ptr<float[]> heapBuf; // large ones go to the heap

        if (g_config.renderRes && g_bbWidth > 0 && g_bbHeight > 0)
        {
            // most batches contain nothing to patch, so the batch is only copied once the first match shows up
            float* patched = nullptr;
            for (UINT i = 0; i < Vector4fCount; i++)
            {
                float v[4];
                memcpy(v, pData + i * 4, sizeof(v));
                if (!FixTexelSizeVec4(v)) continue;

                if (!patched)
                {
                    if (Vector4fCount <= 32)
                    {
                        patched = stackBuf;
                    }
                    else
                    {
                        heapBuf.reset(new float[Vector4fCount * 4]);
                        patched = heapBuf.get();
                    }
                    memcpy(patched, pData, Vector4fCount * 4 * sizeof(float));
                }
                memcpy(patched + i * 4, v, sizeof(v));
            }
            if (patched) forwarded = patched;
        }

        ShaderTweaks::NoteConstants(StartRegister, forwarded, Vector4fCount);

        if (StateCache::RedundantPsConstF(StartRegister, forwarded, Vector4fCount))
            return D3D_OK;
        return Forwarded(orig_SetPixelShaderConstantF(pDevice, StartRegister, forwarded, Vector4fCount));
    }

    // -------------------------------------------------------------------------
    // Hooks: render state and the remaining shader constant calls - only here to drop repeats (see state_cache.h)
    // -------------------------------------------------------------------------
    inline HRESULT WINAPI hook_SetRenderState(IDirect3DDevice9* pDevice, D3DRENDERSTATETYPE State, DWORD Value)
    {
        if (StateCache::RedundantRenderState(State, Value))
            return D3D_OK;
        return Forwarded(orig_SetRenderState(pDevice, State, Value));
    }

    inline HRESULT WINAPI hook_SetVertexShaderConstantF(IDirect3DDevice9* pDevice, UINT StartRegister, const float* pData, UINT Vector4fCount)
    {
        if (StateCache::RedundantVsConstF(StartRegister, pData, Vector4fCount))
            return D3D_OK;
        return Forwarded(orig_SetVertexShaderConstantF(pDevice, StartRegister, pData, Vector4fCount));
    }

    inline HRESULT WINAPI hook_SetVertexShaderConstantB(IDirect3DDevice9* pDevice, UINT StartRegister, const BOOL* pData, UINT BoolCount)
    {
        if (StateCache::RedundantVsConstB(StartRegister, pData, BoolCount))
            return D3D_OK;
        return Forwarded(orig_SetVertexShaderConstantB(pDevice, StartRegister, pData, BoolCount));
    }

    inline HRESULT WINAPI hook_SetPixelShaderConstantB(IDirect3DDevice9* pDevice, UINT StartRegister, const BOOL* pData, UINT BoolCount)
    {
        if (StateCache::RedundantPsConstB(StartRegister, pData, BoolCount))
            return D3D_OK;
        return Forwarded(orig_SetPixelShaderConstantB(pDevice, StartRegister, pData, BoolCount));
    }

    // -------------------------------------------------------------------------
    // Hook: CreateTexture - scale internal scene RTs to backbuffer size & clear texture replacement pointers
    // -------------------------------------------------------------------------
    inline HRESULT WINAPI hook_CreateTexture(IDirect3DDevice9* pDevice, UINT Width, UINT Height, UINT Levels,
        DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture9** ppTexture, HANDLE* pSharedHandle)
    {
        const UINT reqW = Width, reqH = Height; // what the game asked for, before scaling

        if ((Usage & D3DUSAGE_RENDERTARGET) && g_config.renderRes && g_bbWidth > 0 && g_bbHeight > 0)
            TryScaleDimensions(Width, Height);

        HRESULT hr = orig_CreateTexture(pDevice, Width, Height, Levels, Usage, Format, Pool, ppTexture, pSharedHandle);

        if (SUCCEEDED(hr) && ppTexture && *ppTexture)
        {
            AsyncReadback::Install(*ppTexture);
            TextureMods::g_ptrToHash.erase(*ppTexture); // evict any stale entry at this address
            RenderTargets::OnTextureCreated(*ppTexture, reqW, reqH, Usage);
        }

        return hr;
    }

    // -------------------------------------------------------------------------
    // Hook: CreateDepthStencilSurface - scale internal depth surfaces
    // -------------------------------------------------------------------------
    inline HRESULT WINAPI hook_CreateDepthStencilSurface(IDirect3DDevice9* pDevice, UINT Width, UINT Height,
        D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Discard,
        IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle)
    {
        const UINT reqW = Width, reqH = Height;

        if (g_config.renderRes && g_bbWidth > 0 && g_bbHeight > 0)
            TryScaleDimensions(Width, Height);

        HRESULT hr = orig_CreateDepthStencilSurface(pDevice, Width, Height, Format, MultiSample, MultisampleQuality, Discard, ppSurface, pSharedHandle);

        if (SUCCEEDED(hr) && ppSurface && *ppSurface)
            RenderTargets::OnDepthSurfaceCreated(*ppSurface, reqW, reqH);

        return hr;
    }

    // -------------------------------------------------------------------------
    // Hook: SetViewport - scale internal scene viewports
    // -------------------------------------------------------------------------
    inline HRESULT WINAPI hook_SetViewport(IDirect3DDevice9* pDevice, const D3DVIEWPORT9* pViewport)
    {
        if (pViewport && g_config.renderRes && g_bbWidth > 0 && g_bbHeight > 0) {
            UINT w = pViewport->Width, h = pViewport->Height;
            if (TryScaleDimensions(w, h)) {
                D3DVIEWPORT9 vp = *pViewport;
                vp.Width  = w;
                vp.Height = h;
                return orig_SetViewport(pDevice, &vp);
            }
        }
        return orig_SetViewport(pDevice, pViewport);
    }

    // -------------------------------------------------------------------------
    // Hook: SetScissorRect - scale internal scene scissor rects
    // -------------------------------------------------------------------------
    inline HRESULT WINAPI hook_SetScissorRect(IDirect3DDevice9* pDevice, const RECT* pRect)
    {
        if (pRect && g_config.renderRes && g_bbWidth > 0 && g_bbHeight > 0) {
            UINT w = (UINT)(pRect->right  - pRect->left);
            UINT h = (UINT)(pRect->bottom - pRect->top);
            if (TryScaleDimensions(w, h)) {
                RECT r = { 0, 0, (LONG)w, (LONG)h };
                return orig_SetScissorRect(pDevice, &r);
            }
        }
        return orig_SetScissorRect(pDevice, pRect);
    }

    // -------------------------------------------------------------------------
    // Hook: SetRenderTarget - track render target changes
    // -------------------------------------------------------------------------
    inline HRESULT WINAPI hook_SetRenderTarget(IDirect3DDevice9* pDevice, DWORD RenderTargetIndex, IDirect3DSurface9* pRenderTarget)
    {
        const HRESULT hr = orig_SetRenderTarget(pDevice, RenderTargetIndex, pRenderTarget);

        if (SUCCEEDED(hr) && RenderTargetIndex == 0)
            UiHider::g_rt0.store(pRenderTarget, std::memory_order_relaxed);

        if (SUCCEEDED(hr) && pRenderTarget && RenderTargets::g_tracking)
            RenderTargets::OnSetRenderTarget(RenderTargetIndex, pRenderTarget);

        return hr;
    }

    inline HRESULT WINAPI hook_CreatePixelShader(IDirect3DDevice9* pDevice, const DWORD* pFunction, IDirect3DPixelShader9** ppShader)
    {
        const HRESULT hr = orig_CreatePixelShader(pDevice, pFunction, ppShader);
        if (SUCCEEDED(hr) && ppShader && *ppShader)
            ShaderTracker::OnPixelShaderCreated(*ppShader, pFunction);
        return hr;
    }

    inline HRESULT WINAPI hook_SetPixelShader(IDirect3DDevice9* pDevice, IDirect3DPixelShader9* pShader)
    {
        // game may hand one of our patched shaders back to us; everything below must see the game's original object
        pShader = ShaderPatches::ToGame(pShader);

        // game's water / light shaders are swapped for our patched copy at bind time, everything below keeps seeing the original
        const HRESULT hr = orig_SetPixelShader(pDevice, ShaderPatches::Resolve(pDevice, pShader));
        if (SUCCEEDED(hr))
        {
            ShaderTracker::g_curPS.store(pShader, std::memory_order_relaxed);
            ShaderTweaks::OnSetPixelShader(pShader);
        }
        return hr;
    }

    // draw calls that must not be executed: shaders hidden in the diagnostics window, and the in-game UI when it is switched off
    inline bool ShouldSkipDraw()
    {
        return ShaderTracker::ShouldSkipDraw() || UiHider::ShouldSkipDraw(ShaderTracker::g_curPS.load(std::memory_order_relaxed));
    }

    // everything that has to happen right before a draw call: constants for the shader patches and the constant tweaks.
    // Returns what ShaderTweaks::PostDraw has to put back afterwards.
    inline ShaderTweaks::Restore BeforeDraw(IDirect3DDevice9* d)
    {
        const void* ps = ShaderTracker::g_curPS.load(std::memory_order_relaxed);

        // at most one of the two applies to a given shader
        if (ShaderTweaks::Restore patch = ShaderPatches::PreDraw(d, ps); patch.n)
            return patch;
        return ShaderTweaks::PreDraw(d);
    }

    // steps shared by all four draw calls; `draw` performs the actual call and returns its HRESULT
    template <class DrawFn>
    inline HRESULT HookedDraw(IDirect3DDevice9* d, DrawFn&& draw)
    {
        if (ShouldSkipDraw()) return D3D_OK;
        if (RenderTargets::g_tracking) ShaderTracker::OnDraw();
        const ShaderTweaks::Restore restore = BeforeDraw(d);
        const HRESULT hr = draw();
        ShaderTweaks::PostDraw(d, restore);
        if (restore.n)
        {
            // mod changed pixel shader constants behind the state cache: only those registers are unknown now
            for (uint8_t i = 0; i < restore.n; ++i)
                StateCache::InvalidatePsConstF(restore.ranges[i].start, restore.ranges[i].count);
        }
        return hr;
    }

    inline HRESULT WINAPI hook_DrawPrimitive(IDirect3DDevice9* d, D3DPRIMITIVETYPE type, UINT start, UINT count)
    {
        return HookedDraw(d, [&] { return orig_DrawPrimitive(d, type, start, count); });
    }

    inline HRESULT WINAPI hook_DrawIndexedPrimitive(IDirect3DDevice9* d, D3DPRIMITIVETYPE type, INT baseVertex, UINT minVertex, UINT numVertices, UINT startIndex, UINT primCount)
    {
        return HookedDraw(d, [&] {
            return orig_DrawIndexedPrimitive(d, type, baseVertex, minVertex, numVertices, startIndex, primCount);
        });
    }

    inline HRESULT WINAPI hook_DrawPrimitiveUP(IDirect3DDevice9* d, D3DPRIMITIVETYPE type, UINT count, const void* data, UINT stride)
    {
        return HookedDraw(d, [&] { return orig_DrawPrimitiveUP(d, type, count, data, stride); });
    }

    inline HRESULT WINAPI hook_DrawIndexedPrimitiveUP(IDirect3DDevice9* d, D3DPRIMITIVETYPE type, UINT minVertex, UINT numVertices, UINT primCount, const void* indexData,
                                                      D3DFORMAT indexFormat, const void* vertexData, UINT stride)
    {
        return HookedDraw(d, [&] {
            return orig_DrawIndexedPrimitiveUP(d, type, minVertex, numVertices, primCount, indexData, indexFormat, vertexData, stride);
        });
    }

    // -------------------------------------------------------------------------
    // Hook: SetTexture - handle texture replacement
    // -------------------------------------------------------------------------
    inline HRESULT WINAPI hook_SetTexture(IDirect3DDevice9* pDevice, DWORD Stage, IDirect3DBaseTexture9* pTex)
    {
        RenderTargets::NoteStageTexture(Stage, pTex);
        if (pTex && RenderTargets::g_tracking)
            RenderTargets::OnSetTexture(pTex);

        if (pTex && (g_config.textureMods || g_config.textureDump || g_config.textureCycler))
        {
            pTex = TextureMods::HandleSetTexture(pDevice, Stage, pTex);
        }
        return orig_SetTexture(pDevice, Stage, pTex);
    }

    // -------------------------------------------------------------------------
    // Hook: SetSamplerState - anisotropic filtering + LOD bias
    // -------------------------------------------------------------------------
    // every sampler state write of the mod goes through here, so the state cache sees all of them
    inline HRESULT SetSamplerStateCached(IDirect3DDevice9* pDevice, DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value)
    {
        if (StateCache::RedundantSamplerState(Sampler, Type, Value))
            return D3D_OK;
        return Forwarded(orig_SetSamplerState(pDevice, Sampler, Type, Value));
    }

    inline HRESULT WINAPI hook_SetSamplerState(IDirect3DDevice9* pDevice, DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value)
    {
        if (g_config.anisotropy > 1) {
            if (Type == D3DSAMP_MINFILTER && Value >= D3DTEXF_POINT) {
                // sampler 2 causes green & red tint artifacts (pixels) with AF
                // this leaves some textures (like a ground grass texture in the hub) without AF, but better like this than the artifacts
                if (Sampler != 2) {
                    SetSamplerStateCached(pDevice, Sampler, D3DSAMP_MAXANISOTROPY, (DWORD)g_config.anisotropy);
                    SetSamplerStateCached(pDevice, Sampler, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
                    Value = D3DTEXF_ANISOTROPIC;
                }
            }
            // prevent game from downgrading
            if (Type == D3DSAMP_MIPFILTER && Sampler != 2 && Value < D3DTEXF_LINEAR)
                Value = D3DTEXF_LINEAR;
            if (Type == D3DSAMP_MAXANISOTROPY && Sampler != 2 && (DWORD)g_config.anisotropy > Value)
                Value = (DWORD)g_config.anisotropy;
        }

        if (g_config.lodBias < 0.0f) {
            if (Type == D3DSAMP_MINFILTER && Value >= D3DTEXF_POINT)
                SetSamplerStateCached(pDevice, Sampler, D3DSAMP_MIPMAPLODBIAS,
                    *reinterpret_cast<const DWORD*>(&g_config.lodBias));

            // prevent game from downgrading
            if (Type == D3DSAMP_MIPMAPLODBIAS) {
                float incoming = *reinterpret_cast<const float*>(&Value);
                if (incoming > g_config.lodBias)
                    Value = *reinterpret_cast<const DWORD*>(&g_config.lodBias);
            }
        }

        return SetSamplerStateCached(pDevice, Sampler, Type, Value);
    }

    // -------------------------------------------------------------------------
    // Window diagnostics (multi-monitor / mixed DPI debugging)
    // -------------------------------------------------------------------------
    inline void LogWindowDiagnostics(const char* tag)
    {
        if (g_config.logLevel < LogLevel::DEBUG)
            return;

        WindowHooks::LogWindowState(tag, g_hFocusWindow);
        LogDebug("[WinDiag:%s] backbuffer %ux%u (what the mod asked for)", tag, g_bbWidth, g_bbHeight);

        // what the device really has, which can differ from what was requested
        if (g_d3dDevice)
        {
            D3DDEVICE_CREATION_PARAMETERS cp = {};
            if (SUCCEEDED(g_d3dDevice->GetCreationParameters(&cp)))
                LogDebug("[WinDiag:%s] device adapter ordinal %u, focus window %p", tag, cp.AdapterOrdinal, static_cast<void*>(cp.hFocusWindow));

            IDirect3DSurface9* bb = nullptr;
            if (SUCCEEDED(g_d3dDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &bb)) && bb)
            {
                D3DSURFACE_DESC d = {};
                bb->GetDesc(&d);
                LogDebug("[WinDiag:%s] actual backbuffer %ux%u", tag, d.Width, d.Height);
                bb->Release();
            }

            IDirect3DSwapChain9* sc = nullptr;
            if (SUCCEEDED(g_d3dDevice->GetSwapChain(0, &sc)) && sc)
            {
                D3DPRESENT_PARAMETERS spp = {};
                if (SUCCEEDED(sc->GetPresentParameters(&spp)))
                    LogDebug("[WinDiag:%s] swapchain params %ux%u windowed=%d deviceWindow=%p interval=0x%X", tag,
                        spp.BackBufferWidth, spp.BackBufferHeight, spp.Windowed, static_cast<void*>(spp.hDeviceWindow), spp.PresentationInterval);
                sc->Release();
            }

            D3DVIEWPORT9 vp = {};
            if (SUCCEEDED(g_d3dDevice->GetViewport(&vp)))
                LogDebug("[WinDiag:%s] current viewport (%lu,%lu) %lux%lu", tag, vp.X, vp.Y, vp.Width, vp.Height);
        }
    }

    // -------------------------------------------------------------------------
    // Hook: Present - FPS limiter, window setup, init deferred hooks + execute game hooks
    // -------------------------------------------------------------------------
    inline HRESULT WINAPI hook_Present(IDirect3DDevice9* pDevice, const RECT* pSourceRect, const RECT* pDestRect,
        HWND hDestWindowOverride, const RGNDATA* pDirtyRegion)
    {
        static bool s_windowSetup = false;
        if (!s_windowSetup) {
            WindowHooks::TrySetupGameWindow();
            s_windowSetup = (WindowHooks::g_hGameWindow != nullptr);
        }

        // window state snapshots: first frame with a window & later once game has settled (also after every Reset)
        static int s_diagFrame = 0;
        if (s_windowSetup && g_mainThreadId == GetCurrentThreadId()) {
            ++s_diagFrame;
            if (s_diagFrame == 1) LogWindowDiagnostics("first frame");
            else if (s_diagFrame == 600) LogWindowDiagnostics("frame 600");
        }

        // LoadScreen runs on its own thread (Update/Draw/Sleep(29)) while the main thread loads
        // -> feed its stale frameCorrector real time, and skip limiter + game writes (main thread is mid-load)
        if (g_mainThreadId && GetCurrentThreadId() != g_mainThreadId) {
            Game::LoadScreenFix::Update();
            return orig_Present(pDevice, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
        }

        RenderTargets::OnPresent();
        ShaderTracker::OnPresent();
        UiHider::OnPresent();
        StateCache::OnPresent();

        // skip limiter if vsync is on & cap is at or above the refresh rate (vsync already does the job in that case)
        bool limiterRedundant = g_config.vsync && g_displayRefreshHz > 0 && g_config.fpsCap >= g_displayRefreshHz;

        if (g_config.fpsCap > 0 && !limiterRedundant) {
            static HANDLE   s_timer    = nullptr;
            static LONGLONG s_deadline = 0;

            if (!s_timer) {
                s_timer = CreateWaitableTimerExW(nullptr, nullptr,
                    CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
                if (!s_timer)
                    s_timer = CreateWaitableTimerExW(nullptr, nullptr, 0, TIMER_ALL_ACCESS);
                if (s_timer) {
                    LARGE_INTEGER now, freq;
                    QueryPerformanceCounter(&now);
                    QueryPerformanceFrequency(&freq);
                    s_deadline = (now.QuadPart * 10000000LL) / freq.QuadPart;
                }
            }

            if (s_timer) {
                LARGE_INTEGER freq, now;
                QueryPerformanceFrequency(&freq);
                QueryPerformanceCounter(&now);

                const LONGLONG frameInterval100ns = 10000000LL / g_config.fpsCap;
                s_deadline += frameInterval100ns;

                const LONGLONG now100ns = (now.QuadPart * 10000000LL) / freq.QuadPart;
                if (now100ns - s_deadline > frameInterval100ns * 4)
                    s_deadline = now100ns;

                const LONGLONG remaining = s_deadline - now100ns;
                if (remaining > 0) {
                    LARGE_INTEGER due;
                    due.QuadPart = -remaining;
                    SetWaitableTimerEx(s_timer, &due, 0, nullptr, nullptr, nullptr, 0);
                    WaitForSingleObject(s_timer, INFINITE);
                }
            }
        }

        // handle texture mods
        if (TextureMods::g_reloadPending)
        {
            TextureMods::g_reloadPending = false;
            TextureMods::Reload(pDevice);
        }

        if ((g_config.textureMods || g_config.textureDump || g_config.textureCycler) && !TextureMods::g_loaded)
            TextureMods::Load(pDevice);

        if (g_config.textureCycler)
        {
            static bool s_prevPgUp = false, s_prevPgDn = false, s_prevDump = false;
            const bool pgUp = (GetAsyncKeyState(VK_PRIOR) & 0x8000) != 0;
            const bool pgDn = (GetAsyncKeyState(VK_NEXT) & 0x8000) != 0;
            const bool dump = (GetAsyncKeyState(VK_F9) & 0x8000) != 0;

            if (pgUp && !s_prevPgUp) TextureMods::SelectPrev();
            if (pgDn && !s_prevPgDn) TextureMods::SelectNext();
            if (dump && !s_prevDump) TextureMods::DumpSelected();

            s_prevPgUp = pgUp;
            s_prevPgDn = pgDn;
            s_prevDump = dump;

            TextureMods::ClearFrameList(); // clear AFTER hotkeys so keys see the current frame's list
        }

        // initialize game hooks on first rendered frame (to bypass SecuROM)
        if (!g_deferredHooksActive)
            InitDeferredHooks();

        // game "hooks", SecuROM is like "nuh-uh" for actual hooks, so let's modify data instead >:)
        Game::ConstantLookup::Update();
        Game::LuaTables::Update();
        Game::RenderList::Update();
        Game::Difficulty::Update();
        Game::CinemaSkip::Update();
        Game::SkylanderSettings::Update();
        Game::Movement::Update();
        Game::Jump::Update();
        Game::ItemMods::Update();
        Game::FreeCam::Update();
        Game::Music::Update();
        Game::AttackLights::Update();

        return orig_Present(pDevice, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
    }

    inline bool g_imgui_initialized = false;

    // -------------------------------------------------------------------------
    // Hook: EndScene - imgui
    // -------------------------------------------------------------------------
    inline HRESULT WINAPI hook_EndScene(IDirect3DDevice9* pDevice)
    {
        g_d3dDevice = pDevice;

        // 1. initialize imgui
        if (!g_imgui_initialized) {
            D3DDEVICE_CREATION_PARAMETERS params;
            if (SUCCEEDED(pDevice->GetCreationParameters(&params)))
            {
                // initialize imgui
                UI::Get()->Initialize(params.hFocusWindow);
                g_hFocusWindow = params.hFocusWindow;
                ImGui_ImplWin32_Init(params.hFocusWindow);
                ImGui_ImplDX9_Init(pDevice);

                g_imgui_initialized = true;
                Log("[D3D9] ImGui Context & Window safely initialized in EndScene");
            }
        }

        if (g_imgui_initialized && ImGui::GetCurrentContext() != nullptr) {
            // 2. start the ImGui frame
            ImGui_ImplDX9_NewFrame();
            ImGui_ImplWin32_NewFrame();

            ImGui::NewFrame();

            // 3. draw UI
            UI::Get()->Render();

            // 4. finalize and render ImGui to the screen (only if there's something to draw)
            ImGui::Render();
            ImGui::EndFrame();
            ImDrawData* drawData = ImGui::GetDrawData();
            if (drawData && drawData->TotalVtxCount > 0)
            {
                UiHider::g_inOverlay = true; // the mod's own overlay must never be hidden
                ImGui_ImplDX9_RenderDrawData(drawData);
                UiHider::g_inOverlay = false;
                StateCache::Invalidate(); // the ImGui backend changes device state and restores it behind the hooks
            }
        }

        // 5. call original EndScene
        return orig_EndScene(pDevice);
    }

    // -------------------------------------------------------------------------
    // Hook: Reset
    // -------------------------------------------------------------------------
    inline HRESULT WINAPI hook_Reset(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pp)
    {
        ApplyPresentParams(pp);

        // tell ImGui to invalidate its DirectX resources before the reset happens
        if (g_imgui_initialized && ImGui::GetCurrentContext() != nullptr) {
            ImGui_ImplDX9_InvalidateDeviceObjects();
        }

        ShaderTracker::OnReset();
        UiHider::OnReset();
        AsyncReadback::OnReset();
        RenderTargets::OnReset();
        TextureMods::OnReset();

        StateCache::Invalidate();
        HRESULT hr = orig_Reset(pDevice, pp);
        StateCache::Invalidate();

        // tell ImGui to rebuild its DirectX resources after the reset succeeds
        if (SUCCEEDED(hr) && g_imgui_initialized && ImGui::GetCurrentContext() != nullptr) {
            ImGui_ImplDX9_CreateDeviceObjects();
        }

        if (SUCCEEDED(hr))
            LogWindowDiagnostics("after Reset");

        return hr;
    }

    // D3D9 adapter ordinal that drives the target monitor
    inline UINT FindAdapter(IDirect3D9* d3d, UINT requested)
    {
        const HMONITOR want = Monitor::Get().handle;
        if (!want) return requested;
        for (UINT i = 0; i < d3d->GetAdapterCount(); ++i)
            if (d3d->GetAdapterMonitor(i) == want) return i;
        return requested;
    }

    // -------------------------------------------------------------------------
    // Hook: CreateDevice - override params, then hook the returned device vtable
    // -------------------------------------------------------------------------
    inline HRESULT WINAPI hook_CreateDevice(IDirect3D9* pD3D, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow,
        DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pp, IDirect3DDevice9** ppDevice)
    {
        Adapter = FindAdapter(pD3D, Adapter);
        Log("[D3D9] Using adapter %u", Adapter);
        ApplyPresentParams(pp);

        HRESULT hr = orig_CreateDevice(pD3D, Adapter, DeviceType, hFocusWindow, BehaviorFlags, pp, ppDevice);

        if (SUCCEEDED(hr) && ppDevice && *ppDevice) {
            Log("[D3D9] Device behavior flags: 0x%08lX", BehaviorFlags);

            // save display refresh rate
            if (g_displayRefreshHz == 0) {
                D3DDISPLAYMODE mode;
                if (SUCCEEDED(pD3D->GetAdapterDisplayMode(Adapter, &mode))) {
                    g_displayRefreshHz = (int)mode.RefreshRate;
                    Log("[D3D9] Display refresh rate detected: %d Hz", g_displayRefreshHz);
                }
            }

            // save main thread id for load screen fixes
            if (!g_mainThreadId) {
                g_mainThreadId = GetCurrentThreadId();
                Log("[D3D9] Main render thread: %lu", g_mainThreadId);
            }
            StateCache::g_ownerThread = g_mainThreadId;

            static bool s_device_hooked = false;
            if (!s_device_hooked) {
                void** vt = *(void***)(*ppDevice);

                struct DeviceHook {
                    int         index;
                    void*       hook;
                    void**      orig;
                    const char* name;
                };

                DeviceHook hooks[] = {
                    {16,    (void*)&hook_Reset,                     (void**)&orig_Reset,                    "Reset"                     },
                    {17,    (void*)&hook_Present,                   (void**)&orig_Present,                  "Present"                   },
                    {23,    (void*)&hook_CreateTexture,             (void**)&orig_CreateTexture,            "CreateTexture"             },
                    {29,    (void*)&hook_CreateDepthStencilSurface, (void**)&orig_CreateDepthStencilSurface,"CreateDepthStencilSurface" },
                    {32,    (void*)&AsyncReadback::hook_GetRenderTargetData, (void**)&AsyncReadback::orig_GetRenderTargetData, "GetRenderTargetData"},
                    {37,    (void*)&hook_SetRenderTarget,           (void**)&orig_SetRenderTarget,          "SetRenderTarget"           },
                    {42,    (void*)&hook_EndScene,                  (void**)&orig_EndScene,                 "EndScene"                  },
                    {47,    (void*)&hook_SetViewport,               (void**)&orig_SetViewport,              "SetViewport"               },
                    {65,    (void*)&hook_SetTexture,                (void**)&orig_SetTexture,               "SetTexture"                },
                    {69,    (void*)&hook_SetSamplerState,           (void**)&orig_SetSamplerState,          "SetSamplerState"           },
                    {75,    (void*)&hook_SetScissorRect,            (void**)&orig_SetScissorRect,           "SetScissorRect"            },
                    {109,   (void*)&hook_SetPixelShaderConstantF,   (void**)&orig_SetPixelShaderConstantF,  "SetPixelShaderConstantF"   },
                    {81,    (void*)&hook_DrawPrimitive,             (void**)&orig_DrawPrimitive,            "DrawPrimitive"             },
                    {82,    (void*)&hook_DrawIndexedPrimitive,      (void**)&orig_DrawIndexedPrimitive,     "DrawIndexedPrimitive"      },
                    {83,    (void*)&hook_DrawPrimitiveUP,           (void**)&orig_DrawPrimitiveUP,          "DrawPrimitiveUP"           },
                    {84,    (void*)&hook_DrawIndexedPrimitiveUP,    (void**)&orig_DrawIndexedPrimitiveUP,   "DrawIndexedPrimitiveUP"    },
                    {106,   (void*)&hook_CreatePixelShader,         (void**)&orig_CreatePixelShader,        "CreatePixelShader"         },
                    {107,   (void*)&hook_SetPixelShader,            (void**)&orig_SetPixelShader,           "SetPixelShader"            },
                    {57,    (void*)&hook_SetRenderState,            (void**)&orig_SetRenderState,           "SetRenderState"            },
                    {94,    (void*)&hook_SetVertexShaderConstantF,  (void**)&orig_SetVertexShaderConstantF, "SetVertexShaderConstantF" },
                    {98,    (void*)&hook_SetVertexShaderConstantB,  (void**)&orig_SetVertexShaderConstantB, "SetVertexShaderConstantB" },
                    {113,   (void*)&hook_SetPixelShaderConstantB,   (void**)&orig_SetPixelShaderConstantB,  "SetPixelShaderConstantB"  },
                };

                bool allOk = true;
                for (auto& h : hooks) {
                    if (MH_CreateHook(vt[h.index], h.hook, h.orig) == MH_OK) {
                        MH_EnableHook(vt[h.index]);
                        Log("[D3D9] Hooked IDirect3DDevice9::%s", h.name);
                    } else {
                        Log("[D3D9] Failed to hook IDirect3DDevice9::%s", h.name);
                        allOk = false;
                    }
                }

                ShaderTweaks::g_setPSConstF = orig_SetPixelShaderConstantF;
                ShaderPatcher::g_createPS = orig_CreatePixelShader;

                s_device_hooked = true; // always, even on partial failure
                if (!allOk)
                    Log("[D3D9] Warning: some device hooks failed");
            }
        }
        return hr;
    }

    // -------------------------------------------------------------------------
    // Hook: Direct3DCreate9
    // -------------------------------------------------------------------------
    inline IDirect3D9* WINAPI hook_Direct3DCreate9(UINT SDKVersion)
    {
        LogDebug("[D3D9] Direct3DCreate9 intercepted");
        IDirect3D9* pD3D = orig_Direct3DCreate9(SDKVersion);

        if (pD3D) {
            static bool s_create_device_hooked = false;
            if (!s_create_device_hooked) {
                void** vt = *(void***)pD3D;
                if (MH_CreateHook(vt[16], (void*)&hook_CreateDevice, (void**)&orig_CreateDevice) == MH_OK) {
                    MH_EnableHook(vt[16]);
                    Log("[D3D9] Hooked IDirect3D9::CreateDevice");
                    s_create_device_hooked = true;
                }
            }
        }
        return pD3D;
    }

    // -------------------------------------------------------------------------
    // Init
    // -------------------------------------------------------------------------
    inline bool InitD3D9Hooks()
    {
        Log("[D3D9] Hooking Direct3DCreate9 API entry point...");
        MH_STATUS s = MH_CreateHookApiEx(L"d3d9.dll", "Direct3DCreate9",
            &hook_Direct3DCreate9, (void**)&orig_Direct3DCreate9, nullptr);
        if (s != MH_OK) {
            Log("[D3D9] Failed to hook Direct3DCreate9: %d", s);
            return false;
        }
        Log("[D3D9] Waiting for game to call Direct3DCreate9...");
        return true;
    }
}
