#pragma once

#include <Windows.h>
#include <d3d9.h>

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

// Inventory of the game's render targets / depth surfaces
namespace ssa::RenderTargets
{
    enum class Kind : uint8_t { Texture, Surface, DepthSurface };

    struct Entry
    {
        const void* ptr = nullptr; // identity only
        Kind kind = Kind::Texture;
        bool released = false; // address was reused by a newer resource

        UINT reqW = 0, reqH = 0; // size the game asked for
        UINT w = 0, h = 0; // size actually created (after resolution scaling)
        DWORD usage = 0;
        D3DFORMAT format = D3DFMT_UNKNOWN;

        uint32_t binds = 0, samples = 0; // current frame (partial)
        uint32_t bindsLast = 0, samplesLast = 0; // previous complete frame
        uint64_t totalBinds = 0, totalSamples = 0; // since tracking was switched on / counters reset
    };

    // -------------------------------------------------------------------------
    // state
    // -------------------------------------------------------------------------
    inline std::mutex g_mutex; // game renders from two threads during loading screens
    inline std::vector<Entry> g_entries; // creation order, entries are never reused
    inline std::unordered_map<const void*, size_t> g_index; // live pointer -> index into g_entries
    inline std::atomic<bool> g_tracking{false};
    inline const void* g_boundKey = nullptr; // render target currently bound to slot 0 (guarded by g_mutex)
    inline std::atomic<const void*> g_stageKey[16]{}; // texture currently set per sampler stage (lock-free)
    inline void NoteStageTexture(DWORD stage, const void* tex)
    {
        if (stage < 16) g_stageKey[stage].store(tex, std::memory_order_relaxed);
    }

    // -------------------------------------------------------------------------
    // internal
    // -------------------------------------------------------------------------

    // whatever lived at this address is gone: keep its entry in the list as "replaced", stop attributing stats to it
    inline void MarkReleased_NoLock(const void* key)
    {
        if (auto it = g_index.find(key); it != g_index.end())
        {
            g_entries[it->second].released = true;
            g_index.erase(it);
        }
    }

    inline Entry& Register_NoLock(const void* key, Kind kind)
    {
        MarkReleased_NoLock(key);

        g_entries.emplace_back();
        Entry& e = g_entries.back();
        e.ptr = key;
        e.kind = kind;
        g_index[key] = g_entries.size() - 1;
        return e;
    }

    // -------------------------------------------------------------------------
    // hook entry points
    // -------------------------------------------------------------------------

    // called after every successful CreateTexture
    inline void OnTextureCreated(IDirect3DTexture9* tex, UINT reqW, UINT reqH, DWORD usage)
    {
        if (!tex) return;

        if (!(usage & (D3DUSAGE_RENDERTARGET | D3DUSAGE_DEPTHSTENCIL)))
        {
            // ordinary texture: if a dead render target used to live at this address, stop attributing stats to it
            std::lock_guard<std::mutex> lk(g_mutex);
            MarkReleased_NoLock(tex);
            return;
        }

        D3DSURFACE_DESC d{};
        if (FAILED(tex->GetLevelDesc(0, &d))) return;

        std::lock_guard<std::mutex> lk(g_mutex);
        Entry& e = Register_NoLock(tex, Kind::Texture);
        e.reqW = reqW;
        e.reqH = reqH;
        e.w = d.Width;
        e.h = d.Height;
        e.usage = d.Usage;
        e.format = d.Format;
    }

    // called after every successful CreateDepthStencilSurface
    inline void OnDepthSurfaceCreated(IDirect3DSurface9* surf, UINT reqW, UINT reqH)
    {
        if (!surf) return;

        D3DSURFACE_DESC d{};
        if (FAILED(surf->GetDesc(&d))) return;

        std::lock_guard<std::mutex> lk(g_mutex);
        Entry& e = Register_NoLock(surf, Kind::DepthSurface);
        e.reqW = reqW;
        e.reqH = reqH;
        e.w = d.Width;
        e.h = d.Height;
        e.usage = D3DUSAGE_DEPTHSTENCIL;
        e.format = d.Format;
    }

    // called from SetTexture (only while g_tracking)
    inline void OnSetTexture(IDirect3DBaseTexture9* tex)
    {
        std::lock_guard<std::mutex> lk(g_mutex);
        auto it = g_index.find(tex);
        if (it == g_index.end()) return;

        Entry& e = g_entries[it->second];
        ++e.samples;
        ++e.totalSamples;
    }

    // called from SetRenderTarget (only while g_tracking)
    inline void OnSetRenderTarget(DWORD index, IDirect3DSurface9* surf)
    {
        if (index != 0 || !surf) return;

        // a render target texture's surface knows its parent texture; the backbuffer / CreateRenderTarget surfaces don't
        const void* key = surf;
        Kind kind = Kind::Surface;
        IDirect3DTexture9* owner = nullptr;
        if (SUCCEEDED(surf->GetContainer(__uuidof(IDirect3DTexture9), reinterpret_cast<void**>(&owner))) && owner)
        {
            key = owner; // address used as identity only
            kind = Kind::Texture;
            owner->Release();
        }

        D3DSURFACE_DESC d{};
        const bool haveDesc = SUCCEEDED(surf->GetDesc(&d));

        std::lock_guard<std::mutex> lk(g_mutex);
        auto it = g_index.find(key);
        if (it == g_index.end())
        {
            // not seen at creation: first noticed when the game bound it
            if (!haveDesc) return;
            Entry& e = Register_NoLock(key, kind);
            e.reqW = e.w = d.Width;
            e.reqH = e.h = d.Height;
            e.usage = d.Usage;
            e.format = d.Format;
            it = g_index.find(key);
        }

        Entry& e = g_entries[it->second];
        ++e.binds;
        ++e.totalBinds;
        g_boundKey = key;
    }

    // called once per frame from Present
    inline void OnPresent()
    {
        if (!g_tracking) return;

        std::lock_guard<std::mutex> lk(g_mutex);
        for (auto& e : g_entries)
        {
            e.bindsLast = e.binds;
            e.samplesLast = e.samples;
            e.binds = 0;
            e.samples = 0;
        }
    }

    // called from hook_Reset BEFORE orig_Reset
    inline void OnReset()
    {
        std::lock_guard<std::mutex> lk(g_mutex);
        g_boundKey = nullptr;
        g_entries.clear();
        g_index.clear();
    }

    // -------------------------------------------------------------------------
    // UI side
    // -------------------------------------------------------------------------
    inline void ResetCounters()
    {
        std::lock_guard<std::mutex> lk(g_mutex);
        for (auto& e : g_entries)
        {
            e.binds = e.samples = e.bindsLast = e.samplesLast = 0;
            e.totalBinds = e.totalSamples = 0;
        }
    }

    // window open -> tracking on (counters start from zero), window closed -> tracking off
    inline void SetTracking(bool on)
    {
        const bool was = g_tracking.exchange(on);
        if (on && !was) ResetCounters();
    }

    // copy for the UI so no lock is held while ImGui draws (ImGui's own draw calls go through our hooks too)
    inline std::vector<Entry> Snapshot()
    {
        std::lock_guard<std::mutex> lk(g_mutex);
        return g_entries;
    }

    inline std::string FormatName(D3DFORMAT f)
    {
#define SSA_FMT(name) { D3DFMT_##name, #name }
        static const struct
        {
            D3DFORMAT format;
            const char* name;
        } kNames[] = {
            SSA_FMT(A8R8G8B8), SSA_FMT(X8R8G8B8), SSA_FMT(R5G6B5), SSA_FMT(A8), SSA_FMT(L8), SSA_FMT(G16R16),
            SSA_FMT(A2R10G10B10), SSA_FMT(A2B10G10R10), SSA_FMT(A16B16G16R16),
            SSA_FMT(R16F), SSA_FMT(G16R16F), SSA_FMT(A16B16G16R16F),
            SSA_FMT(R32F), SSA_FMT(G32R32F), SSA_FMT(A32B32G32R32F),
            SSA_FMT(D16), SSA_FMT(D24X8), SSA_FMT(D24S8), SSA_FMT(D32), SSA_FMT(D24FS8),
        };
#undef SSA_FMT
        for (const auto& n : kNames)
            if (n.format == f) return n.name;

        // FourCC formats such as INTZ / DF24
        const auto v = static_cast<DWORD>(f);
        auto printable = [](DWORD c) { return c >= 32 && c < 127; };
        char buf[24];
        if (printable(v & 0xFF) && printable((v >> 8) & 0xFF) && printable((v >> 16) & 0xFF) && printable(
            (v >> 24) & 0xFF))
            snprintf(buf, sizeof(buf), "%c%c%c%c", (char)(v & 0xFF), (char)((v >> 8) & 0xFF), (char)((v >> 16) & 0xFF),
                     (char)((v >> 24) & 0xFF));
        else
            snprintf(buf, sizeof(buf), "fmt %lu", static_cast<unsigned long>(v));
        return buf;
    }

    inline std::string UsageString(DWORD u)
    {
        std::string s;
        if (u & D3DUSAGE_RENDERTARGET) s += "RT ";
        if (u & D3DUSAGE_DEPTHSTENCIL) s += "DS ";
        if (u & D3DUSAGE_DYNAMIC) s += "dynamic ";
        if (u & D3DUSAGE_AUTOGENMIPMAP) s += "mipgen ";
        if (s.empty()) return "-";
        s.pop_back();
        return s;
    }

    // plain-language usage summary, based on totals since the window was opened / counters were reset
    inline const char* StatusText(const Entry& e)
    {
        if (e.released) return "replaced";

        const bool written = e.totalBinds > 0;
        const bool read = e.totalSamples > 0;

        // depth buffers are bound with SetDepthStencilSurface, which we don't watch yet, so only reads are known
        if (e.kind == Kind::DepthSurface || (e.usage & D3DUSAGE_DEPTHSTENCIL))
            return read ? "depth, sampled" : "depth, writes not tracked";

        if (written && read) return "live";
        if (written) return "write only";
        if (read) return "read only";
        return "unused";
    }
} // namespace ssa::RenderTargets
