#pragma once
#include <Windows.h>
#include <d3d9.h>

#include "config.h"
#include "log.h"
#include "MinHook.h"
#include "window_hooks.h"

namespace ssa::D3D9Hooks
{
    using Direct3DCreate9_t     = IDirect3D9*(WINAPI*)(UINT);
    using CreateDevice_t        = HRESULT(WINAPI*)(IDirect3D9*, UINT, D3DDEVTYPE, HWND, DWORD, D3DPRESENT_PARAMETERS*, IDirect3DDevice9**);
    using Reset_t               = HRESULT(WINAPI*)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);
    using EndScene_t            = HRESULT(WINAPI*)(IDirect3DDevice9*);
    using Present_t             = HRESULT(WINAPI*)(IDirect3DDevice9*, const RECT*, const RECT*, HWND, const RGNDATA*);
    using SetSamplerState_t     = HRESULT(WINAPI*)(IDirect3DDevice9*, DWORD, D3DSAMPLERSTATETYPE, DWORD);

    inline Direct3DCreate9_t    orig_Direct3DCreate9    = nullptr;
    inline CreateDevice_t       orig_CreateDevice       = nullptr;
    inline Reset_t              orig_Reset              = nullptr;
    inline EndScene_t           orig_EndScene           = nullptr;
    inline Present_t            orig_Present            = nullptr;
    inline SetSamplerState_t    orig_SetSamplerState    = nullptr;

    // -------------------------------------------------------------------------
    // Helpers
    // -------------------------------------------------------------------------

    inline int g_displayRefreshHz = 0;

    // parameter override logic
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

        pp->PresentationInterval = g_config.vsync ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;

        Log("[D3D9] Present params: %dx%d windowed=%d vsync=%d",
            pp->BackBufferWidth, pp->BackBufferHeight, pp->Windowed, g_config.vsync);
    }

    // -------------------------------------------------------------------------
    // Hook: SetSamplerState - injects anisotropic filtering
    // -------------------------------------------------------------------------
    inline HRESULT WINAPI hook_SetSamplerState(
        IDirect3DDevice9* pDevice, DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value)
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
            if (Type == D3DSAMP_MIPFILTER && Sampler != 2 && Value < D3DTEXF_LINEAR) {
                Value = D3DTEXF_LINEAR;
            }
            if (Type == D3DSAMP_MAXANISOTROPY && Sampler != 2 && (DWORD)g_config.anisotropy > Value) {
                Value = (DWORD)g_config.anisotropy;
            }
        }

        if (g_config.lodBias < 0.0f) {
            if (Type == D3DSAMP_MINFILTER && Value >= D3DTEXF_POINT) {
                orig_SetSamplerState(pDevice, Sampler, D3DSAMP_MIPMAPLODBIAS, *reinterpret_cast<const DWORD*>(&g_config.lodBias));
            }

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
    // Hook: Present (vtable index 17) - FPS limiter only
    // -------------------------------------------------------------------------
    inline HRESULT WINAPI hook_Present(
        IDirect3DDevice9* pDevice,
        const RECT* pSourceRect, const RECT* pDestRect,
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
                if (!s_timer) {
                    s_timer = CreateWaitableTimerExW(nullptr, nullptr,
                        0, TIMER_ALL_ACCESS);
                    Log("[D3D9] Using standard waitable timer (high-res not available)");
                }

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

        return orig_Present(pDevice, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
    }

    // -------------------------------------------------------------------------
    // Hook: EndScene - (imgui possibly)
    // -------------------------------------------------------------------------
    inline HRESULT WINAPI hook_EndScene(IDirect3DDevice9* pDevice)
    {
        return orig_EndScene(pDevice);
    }

    // -------------------------------------------------------------------------
    // Hook: Reset
    // -------------------------------------------------------------------------
    inline HRESULT WINAPI hook_Reset(
        IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pp)
    {
        ApplyPresentParams(pp);
        return orig_Reset(pDevice, pp);
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
                    {16, (void*)&hook_Reset, (void**)&orig_Reset, "Reset"},
                    {17, (void*)&hook_Present, (void**)&orig_Present, "Present"},
                    // {42, (void*)&hook_EndScene, (void**)&orig_EndScene, "EndScene"}, // TODO: add back when needed
                    {69, (void*)&hook_SetSamplerState, (void**)&orig_SetSamplerState, "SetSamplerState"},
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
    // Hook: Direct3DCreate9 - intercept the IDirect3D9 object
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
        if (s != MH_OK)
        {
            Log("[D3D9] Failed to hook Direct3DCreate9: %d", s);
            return false;
        }
        Log("[D3D9] Waiting for game to call Direct3DCreate9...");
        return true;
    }
}
