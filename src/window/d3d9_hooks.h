#pragma once
#include <Windows.h>
#include <d3d9.h>

#include "config.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#include "log.h"
#include "MinHook.h"
#include "window_hooks.h"
#include "texture_mods.h"
#include "patches.h"
#include "game/difficulty.h"
#include "game/grassPatch.h"

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

    // -------------------------------------------------------------------------
    // state
    // -------------------------------------------------------------------------
    inline int g_displayRefreshHz = 0;
    inline UINT g_bbWidth = 0;
    inline UINT g_bbHeight = 0;
    inline IDirect3DDevice9* g_d3dDevice = nullptr;

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

    inline bool TryScaleDimensions(UINT& w, UINT& h)
    {
        struct Entry { UINT sw, sh, dw, dh; };
        const Entry table[] = {
            { k_internalW,      k_internalH,        SsW(),          SsH()           },  // main scene color RT
            { k_internalW / 2,  k_internalH / 2,    SsW() / 2,      SsH() / 2       },  // half-res post-fx
            { k_internalW / 4,  k_internalH / 4,    SsW() / 4,      SsH() / 4       },  // quarter-res downsample + SSAO
            { 288,              176,                SsW() / 4,      SsH() / 4       },  // bloom quarter ping-pong
            { 144,              96,                 SsW() / 8,      SsH() / 8       },  // bloom eighth blur chain
            { 512,              512,                SsSq(512), SsSq(512)  },  // mirror color RT
            { 256,              256,                SsSq(256), SsSq(256)  },  // mirror blur/VSM target
        };
        for (const auto& e : table) {
            if (w == e.sw && h == e.sh) {
                w = e.dw;
                h = e.dh;
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

        if (g_config.windowed) {
            pp->Windowed = TRUE;
            pp->FullScreen_RefreshRateInHz = 0;
        }

        bool hasCustomRes = g_config.resolutionW > 0 && g_config.resolutionH > 0;

        if (g_config.windowed && g_config.borderless) {
            // always fill the desktop; fall back to desktop res if no custom res set
            pp->BackBufferWidth  = hasCustomRes ? g_config.resolutionW : GetSystemMetrics(SM_CXSCREEN);
            pp->BackBufferHeight = hasCustomRes ? g_config.resolutionH : GetSystemMetrics(SM_CYSCREEN);
        } else if (hasCustomRes) {
            // plain windowed with custom res, or fullscreen with custom res: apply it
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
        // all internal resolution tiers and their scaled equivalents
        struct Tier { UINT iW, iH, oW, oH; };
        const Tier tiers[] = {
            { k_internalW,      k_internalH,        SsW(),          SsH()           },  // main scene color RT
            { k_internalW / 2,  k_internalH / 2,    SsW() / 2,      SsH() / 2       },  // half-res post-fx
            { k_internalW / 4,  k_internalH / 4,    SsW() / 4,      SsH() / 4       },  // quarter-res downsample + SSAO
            { 288,              176,                SsW() / 4,      SsH() / 4       },  // bloom quarter ping-pong
            { 144,              96,                 SsW() / 8,      SsH() / 8       },  // bloom eighth blur chain
            { 512,              512,                SsSq(512), SsSq(512)  },  // mirror color RT
            { 256,              256,                SsSq(256), SsSq(256)  },  // mirror blur/VSM target
        };

        const float eps = 0.01f;
        auto fNear = [&](float a, float b) { return fabsf(a - b) < fabsf(b) * eps; };

        for (const auto& t : tiers) {
            const float iW = 1.f / t.iW;
            const float iH = 1.f / t.iH;

            // pattern: (2/W, 2/H, 1/W, 1/H)
            if (fNear(v[0], 2*iW) && fNear(v[1], 2*iH) && fNear(v[2], iW) && fNear(v[3], iH)) {
                float nW = 1.f/t.oW, nH = 1.f/t.oH;
                v[0] = 2*nW; v[1] = 2*nH; v[2] = nW; v[3] = nH;
                return true;
            }

            // pattern: (1/W, 1/H, 0.5/W, 0.5/H)
            if (fNear(v[0], iW) && fNear(v[1], iH) && fNear(v[2], .5f*iW) && fNear(v[3], .5f*iH)) {
                float nW = 1.f/t.oW, nH = 1.f/t.oH;
                v[0] = nW; v[1] = nH; v[2] = .5f*nW; v[3] = .5f*nH;
                return true;
            }
        }

        return false;
    }

    // -------------------------------------------------------------------------
    // Hook: SetPixelShaderConstantF - patches texel-size uniforms for nativeRes
    // -------------------------------------------------------------------------
    inline HRESULT WINAPI hook_SetPixelShaderConstantF(
        IDirect3DDevice9* pDevice, UINT StartRegister, const float* pData, UINT Vector4fCount)
    {
        if (g_config.renderRes && g_bbWidth > 0 && g_bbHeight > 0) {
            // stack-allocate for small batches (typical), heap for large ones
            float  stackBuf[32 * 4];
            float* patched = (Vector4fCount <= 32)
                ? stackBuf
                : new float[Vector4fCount * 4];

            memcpy(patched, pData, Vector4fCount * 4 * sizeof(float));

            bool any = false;
            for (UINT i = 0; i < Vector4fCount; i++)
                if (FixTexelSizeVec4(patched + i * 4)) any = true;

            HRESULT hr = orig_SetPixelShaderConstantF(pDevice, StartRegister,
                any ? patched : pData, Vector4fCount);

            if (Vector4fCount > 32) delete[] patched;
            return hr;
        }
        return orig_SetPixelShaderConstantF(pDevice, StartRegister, pData, Vector4fCount);
    }

    // -------------------------------------------------------------------------
    // Hook: CreateTexture - scale internal scene RTs to backbuffer size
    // -------------------------------------------------------------------------
    inline HRESULT WINAPI hook_CreateTexture(
        IDirect3DDevice9* pDevice, UINT Width, UINT Height, UINT Levels,
        DWORD Usage, D3DFORMAT Format, D3DPOOL Pool,
        IDirect3DTexture9** ppTexture, HANDLE* pSharedHandle)
    {
        if ((Usage & D3DUSAGE_RENDERTARGET) && g_config.renderRes && g_bbWidth > 0 && g_bbHeight > 0)
            TryScaleDimensions(Width, Height);

        return orig_CreateTexture(pDevice, Width, Height, Levels, Usage, Format, Pool, ppTexture, pSharedHandle);
    }

    // -------------------------------------------------------------------------
    // Hook: CreateDepthStencilSurface - scale internal depth surfaces
    // -------------------------------------------------------------------------
    inline HRESULT WINAPI hook_CreateDepthStencilSurface(
        IDirect3DDevice9* pDevice, UINT Width, UINT Height,
        D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality,
        BOOL Discard, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle)
    {
        if (g_config.renderRes && g_bbWidth > 0 && g_bbHeight > 0)
            TryScaleDimensions(Width, Height);

        return orig_CreateDepthStencilSurface(pDevice, Width, Height, Format,
            MultiSample, MultisampleQuality, Discard, ppSurface, pSharedHandle);
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
    // Hook: SetTexture
    // -------------------------------------------------------------------------
    inline HRESULT WINAPI hook_SetTexture(IDirect3DDevice9* pDevice, DWORD Stage, IDirect3DBaseTexture9* pTex)
    {
        if (pTex && (g_config.textureMods || g_config.textureDump))
        {
            pTex = TextureMods::HandleSetTexture(pDevice, Stage, pTex);
        }
        return orig_SetTexture(pDevice, Stage, pTex);
    }


    // -------------------------------------------------------------------------
    // Hook: SetSamplerState - anisotropic filtering + LOD bias
    // -------------------------------------------------------------------------
    inline HRESULT WINAPI hook_SetSamplerState(IDirect3DDevice9* pDevice, DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value)
    {
        if (g_config.anisotropy > 1) {
            if (Type == D3DSAMP_MINFILTER && Value >= D3DTEXF_POINT) {
                // sampler 2 causes green & red tint artifacts (pixels) with AF
                // this leaves some textures (like a ground grass texture in the hub) without AF, but better like this than the artifacts
                if (Sampler != 2) {
                    orig_SetSamplerState(pDevice, Sampler, D3DSAMP_MAXANISOTROPY, (DWORD)g_config.anisotropy);
                    orig_SetSamplerState(pDevice, Sampler, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
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
                orig_SetSamplerState(pDevice, Sampler, D3DSAMP_MIPMAPLODBIAS,
                    *reinterpret_cast<const DWORD*>(&g_config.lodBias));

            // prevent game from downgrading
            if (Type == D3DSAMP_MIPMAPLODBIAS) {
                float incoming = *reinterpret_cast<const float*>(&Value);
                if (incoming > g_config.lodBias)
                    Value = *reinterpret_cast<const DWORD*>(&g_config.lodBias);
            }
        }

        return orig_SetSamplerState(pDevice, Sampler, Type, Value);
    }

    // -------------------------------------------------------------------------
    // Hook: Present - FPS limiter, window setup, init deferred hooks + execute game hooks
    // -------------------------------------------------------------------------
    inline HRESULT WINAPI hook_Present(IDirect3DDevice9* pDevice, const RECT* pSourceRect, const RECT* pDestRect,
        HWND hDestWindowOverride, const RGNDATA* pDirtyRegion)
    {
        static bool s_windowSetup = false;
        if (!s_windowSetup && g_config.windowed) {
            WindowHooks::TrySetupGameWindow();
            s_windowSetup = (WindowHooks::g_hGameWindow != nullptr);
        }

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

        if ((g_config.textureMods || g_config.textureDump) && !TextureMods::g_loaded)
            TextureMods::Load(pDevice);

        // initialize game hooks on first rendered frame (to bypass SecuROM)
        if (!g_deferredHooksActive)
            InitDeferredHooks();

        // game "hooks", SecuROM is like "nuh-uh" for actual hooks, so let's modify data instead >:)
        Game::GrassPatch::ApplyGrassPatch();
        Difficulty::Update();

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
                UI::Get()->Initialize();
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

            // 4. finalize and render ImGui to the screen
            ImGui::Render();
            ImGui::EndFrame();
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
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

        TextureMods::OnReset();

        HRESULT hr = orig_Reset(pDevice, pp);

        // tell ImGui to rebuild its DirectX resources after the reset succeeds
        if (SUCCEEDED(hr) && g_imgui_initialized && ImGui::GetCurrentContext() != nullptr) {
            ImGui_ImplDX9_CreateDeviceObjects();
        }

        return hr;
    }

    // -------------------------------------------------------------------------
    // Hook: CreateDevice - override params, then hook the returned device vtable
    // -------------------------------------------------------------------------
    inline HRESULT WINAPI hook_CreateDevice(
        IDirect3D9* pD3D, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow,
        DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pp, IDirect3DDevice9** ppDevice)
    {
        ApplyPresentParams(pp);

        HRESULT hr = orig_CreateDevice(pD3D, Adapter, DeviceType, hFocusWindow, BehaviorFlags, pp, ppDevice);

        if (SUCCEEDED(hr) && ppDevice && *ppDevice) {
            // save display refresh rate
            if (g_displayRefreshHz == 0) {
                D3DDISPLAYMODE mode;
                if (SUCCEEDED(pD3D->GetAdapterDisplayMode(Adapter, &mode))) {
                    g_displayRefreshHz = (int)mode.RefreshRate;
                    Log("[D3D9] Display refresh rate detected: %d Hz", g_displayRefreshHz);
                }
            }

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
                    {42,    (void*)&hook_EndScene,                  (void**)&orig_EndScene,                 "EndScene"                  },
                    {47,    (void*)&hook_SetViewport,               (void**)&orig_SetViewport,              "SetViewport"               },
                    {65,    (void*)&hook_SetTexture,                (void**)&orig_SetTexture,               "SetTexture"                },
                    {69,    (void*)&hook_SetSamplerState,           (void**)&orig_SetSamplerState,          "SetSamplerState"           },
                    {75,    (void*)&hook_SetScissorRect,            (void**)&orig_SetScissorRect,           "SetScissorRect"            },
                    {109,   (void*)&hook_SetPixelShaderConstantF,   (void**)&orig_SetPixelShaderConstantF,  "SetPixelShaderConstantF"   },
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
