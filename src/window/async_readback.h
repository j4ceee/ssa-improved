#pragma once
#include <Windows.h>
#include <d3d9.h>
#include <intrin.h>
#include <cstdint>
#include <cstring>
#include "MinHook.h"
#include "addresses.h"
#include "log.h"
#include "state_cache.h"

// Game copies down to a 4x4 texture every frame, reads it back to the CPU and builds a histogram of the 16 pixels
// - lock right after the copy was queued makes the CPU wait: for the GPU on a native driver, for the translation thread of the D3D9 layer on Proton
//
// This makes the readback asynchronous:
// - right before the game queues the next copy into the texture (GetRenderTargetData), the PREVIOUS copy is picked up with a lock that cannot block (D3DLOCK_DONOTWAIT, copy is a whole frame old by then)
// - the game's own lock then never reaches D3D: it is answered with those pixels (a private 64 byte copy), so the histogram is one frame old, and the game's UnlockRect for it is swallowed because the texture was never locked
// - if anything is unexpected (another texture, nothing picked up yet, the pickup keeps failing) the game's lock happens as asked
namespace ssa::AsyncReadback
{
    using LockRect_t = HRESULT(WINAPI*)(IDirect3DTexture9*, UINT, D3DLOCKED_RECT*, const RECT*, DWORD);
    using UnlockRect_t = HRESULT(WINAPI*)(IDirect3DTexture9*, UINT);
    using GetRenderTargetData_t = HRESULT(WINAPI*)(IDirect3DDevice9*, IDirect3DSurface9*, IDirect3DSurface9*);

    inline LockRect_t orig_LockRect = nullptr;
    inline UnlockRect_t orig_UnlockRect = nullptr;
    inline GetRenderTargetData_t orig_GetRenderTargetData = nullptr;

    inline bool g_enabled = true; // switchable in Diagnostics
    inline bool g_installed = false;

    inline constexpr UINT kSize = 4; // the texture is kSize x kSize
    inline constexpr UINT kPitch = kSize * 4; // A8R8G8B8
    inline constexpr uint32_t kMaxMissedPolls = 8;
    // after this many frames without a fresh pickup the game's lock is let through again

    inline IDirect3DTexture9* g_texture = nullptr; // game's readback texture (we hold a reference), found at its first lock
    inline bool g_haveData = false; // g_data holds pixels
    inline bool g_fakeLocked = false; // game's current lock was answered from g_data, its UnlockRect must not reach D3D
    inline uint32_t g_missedPolls = 0; // copies in a row that could not be picked up early
    inline uint8_t g_data[kSize * kPitch] = {};

    // for the diagnostics window
    inline uint32_t g_deferred = 0; // game locks answered with the previous pixels
    inline uint32_t g_blocked = 0; // game locks that had to go through anyway
    inline uint32_t g_polled = 0; // copies picked up early
    inline uint32_t g_missed = 0; // copies that could not be picked up early

    inline bool OnRenderThread() { return GetCurrentThreadId() == StateCache::g_ownerThread; }

    inline void Remember(const D3DLOCKED_RECT& locked)
    {
        if (!locked.pBits)
            return;
        for (UINT y = 0; y < kSize; ++y)
            std::memcpy(g_data + y * kPitch, static_cast<const uint8_t*>(locked.pBits) + y * locked.Pitch, kPitch);
        g_haveData = true;
    }

    // is this the game's lock of the readback texture, and is the texture what we expect it to be
    inline bool IsReadbackLock(IDirect3DTexture9* self, const uint32_t returnAddress, const UINT level, const RECT* rect, const DWORD flags)
    {
        if (!g_enabled)
            return false;
        if (returnAddress != static_cast<uint32_t>(reinterpret_cast<uintptr_t>(GetAddress(READBACK_LOCK_RETURN))))
            return false;
        if (level != 0 || rect || !(flags & D3DLOCK_READONLY))
            return false;

        if (self != g_texture)
        {
            D3DSURFACE_DESC desc = {};
            if (FAILED(self->GetLevelDesc(0, &desc)) || desc.Width != kSize || desc.Height != kSize ||
                desc.Format != D3DFMT_A8R8G8B8 || desc.Pool != D3DPOOL_SYSTEMMEM)
            {
                Log(
                    "[AsyncReadback] the texture locked at the readback site is not the expected 4x4 system memory texture, disabled");
                g_enabled = false;
                return false;
            }
            self->AddRef(); // the early pickup must never see a freed texture
            if (g_texture)
                g_texture->Release();
            g_texture = self;
            g_haveData = false;
            g_fakeLocked = false;
            g_missedPolls = 0;
            Log("[AsyncReadback] readback texture %p found", static_cast<void*>(self));
        }
        return true;
    }

    // the game's lock: the pixels of the last copy that was picked up early, no D3D call
    inline HRESULT AnswerLock(IDirect3DTexture9* self, D3DLOCKED_RECT* locked, const DWORD flags)
    {
        if (g_haveData && g_missedPolls < kMaxMissedPolls)
        {
            locked->pBits = g_data;
            locked->Pitch = kPitch;
            g_fakeLocked = true;
            ++g_deferred;
            return D3D_OK;
        }

        // nothing picked up yet (the first frames), or the pickup keeps failing: lock the way the game asked
        ++g_blocked;
        const HRESULT hr = orig_LockRect(self, 0, locked, nullptr, flags);
        if (SUCCEEDED(hr))
            Remember(*locked);
        return hr;
    }

    inline HRESULT WINAPI hook_LockRect(IDirect3DTexture9* self, UINT level, D3DLOCKED_RECT* locked, const RECT* rect, DWORD flags)
    {
        const auto returnAddress = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(_ReturnAddress()));

        if (!OnRenderThread()) // load screen draws from another thread
            return orig_LockRect(self, level, locked, rect, flags);

        if (locked && IsReadbackLock(self, returnAddress, level, rect, flags))
            return AnswerLock(self, locked, flags);
        return orig_LockRect(self, level, locked, rect, flags);
    }

    inline HRESULT WINAPI hook_UnlockRect(IDirect3DTexture9* self, UINT level)
    {
        if (g_fakeLocked && self == g_texture && level == 0)
        {
            g_fakeLocked = false;
            return D3D_OK;
        }
        return orig_UnlockRect(self, level);
    }

    // is `surface` the top level of the readback texture
    inline bool IsReadbackSurface(IDirect3DSurface9* surface)
    {
        if (!surface || !g_texture)
            return false;
        IDirect3DSurface9* top = nullptr;
        if (FAILED(g_texture->GetSurfaceLevel(0, &top)) || !top)
            return false;
        const bool same = top == surface;
        top->Release();
        return same;
    }

    // picks up the last copy if the GPU is done with it (cannot block)
    inline void Poll()
    {
        D3DLOCKED_RECT locked = {};
        if (SUCCEEDED(orig_LockRect(g_texture, 0, &locked, nullptr, D3DLOCK_READONLY | D3DLOCK_DONOTWAIT)))
        {
            Remember(locked);
            orig_UnlockRect(g_texture, 0);
            g_missedPolls = 0;
            ++g_polled;
        }
        else
        {
            if (g_missedPolls < kMaxMissedPolls)
                ++g_missedPolls;
            ++g_missed;
        }
    }

    // game is about to queue the next copy into the readback texture: pick up the previous one first
    inline HRESULT WINAPI hook_GetRenderTargetData(IDirect3DDevice9* device, IDirect3DSurface9* renderTarget, IDirect3DSurface9* destination)
    {
        if (g_enabled && !g_fakeLocked && OnRenderThread() && IsReadbackSurface(destination))
            Poll();
        return orig_GetRenderTargetData(device, renderTarget, destination);
    }

    // from hook_CreateTexture with any texture the game created: all textures share the LockRect / UnlockRect implementation, so this hooks all of them
    inline void Install(IDirect3DTexture9* texture)
    {
        if (g_installed || !texture)
            return;
        g_installed = true;

        void** vtable = *reinterpret_cast<void***>(texture);
        constexpr int kLockRect = 19; // IDirect3DTexture9::LockRect
        constexpr int kUnlockRect = 20; // IDirect3DTexture9::UnlockRect
        const bool ok =
            MH_CreateHook(vtable[kLockRect], reinterpret_cast<void*>(&hook_LockRect),
                          reinterpret_cast<void**>(&orig_LockRect)) == MH_OK &&
            MH_CreateHook(vtable[kUnlockRect], reinterpret_cast<void*>(&hook_UnlockRect),
                          reinterpret_cast<void**>(&orig_UnlockRect)) == MH_OK &&
            MH_EnableHook(vtable[kLockRect]) == MH_OK && MH_EnableHook(vtable[kUnlockRect]) == MH_OK;
        if (ok)
        {
            Log("[AsyncReadback] hooked IDirect3DTexture9::LockRect / UnlockRect");
        }
        else
        {
            Log("[AsyncReadback] failed to hook IDirect3DTexture9::LockRect / UnlockRect, disabled");
            g_enabled = false;
        }
    }

    // from hook_Reset (before the device is reset)
    inline void OnReset()
    {
        if (g_texture)
        {
            g_texture->Release();
            g_texture = nullptr;
        }
        g_haveData = false;
        g_fakeLocked = false;
        g_missedPolls = 0;
    }
} // namespace ssa::AsyncReadback
