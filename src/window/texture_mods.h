#pragma once
#include <Windows.h>
#include <d3d9.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <filesystem>
#include <cstdint>

#include "DDSTextureLoader9.h"
#include "ScreenGrab9.h"
#include "config.h"
#include "log.h"
#include "texture_format_utils.h"

namespace ssa::TextureMods
{
    namespace fs = std::filesystem;

    inline bool g_reloadPending = false;

    // -------------------------------------------------------------------------
    // state
    // -------------------------------------------------------------------------

    // maps game texture pointer → content hash (0 = unlockable / skip)
    inline std::unordered_map<IDirect3DTexture9*, uint64_t> g_ptrToHash;

    // maps content hash → loaded replacement texture
    // stored as IDirect3DBaseTexture9* (matches DDSTextureLoader9's output type & lets us swap pTex in hook_SetTexture without any cast)
    inline std::unordered_map<uint64_t, IDirect3DBaseTexture9*> g_replacements;

    // hashes already written to disk this session, prevents re-dumping on re-bind
    inline std::unordered_set<uint64_t> g_dumped;

    // maps content hash → surface descriptor (width, height, format)
    // populated at hash time, used by LogSelection() to avoid COM pointer lookups
    // intentionally preserved across resets - hash → desc is stable metadata
    inline std::unordered_map<uint64_t, D3DSURFACE_DESC> g_hashToDesc;

    // set to true once Load() has run
    inline bool g_loaded = false;

    // -------------------------------------------------------------------------
    // Highlight texture for cycler (D3DPOOL_MANAGED -> survives Reset, no release needed)
    // -------------------------------------------------------------------------
    inline IDirect3DBaseTexture9* g_debugTexHighlight = nullptr;

    inline void CreateDebugTextures(IDirect3DDevice9* pDevice)
    {
        if (g_debugTexHighlight) return; // already created

        IDirect3DTexture9* pTex = nullptr;
        if (FAILED(pDevice->CreateTexture(4, 4, 1, 0, D3DFMT_A8R8G8B8,
            D3DPOOL_MANAGED, &pTex, nullptr)) || !pTex)
            return;

        D3DLOCKED_RECT lr;
        if (SUCCEEDED(pTex->LockRect(0, &lr, nullptr, 0)))
        {
            for (int y = 0; y < 4; y++)
            {
                auto* row = reinterpret_cast<uint32_t*>(
                    static_cast<uint8_t*>(lr.pBits) + y * lr.Pitch);
                for (int x = 0; x < 4; x++)
                    row[x] = 0xFFFFFF00; // yellow
            }
            pTex->UnlockRect(0);
        }
        g_debugTexHighlight = pTex;
        LogDebug("[TextureMods] Created highlight texture (yellow)");
    }

    inline void ReleaseDebugTextures()
    {
        // D3DPOOL_MANAGED survives Reset without explicit release, but we release on shutdown
        if (g_debugTexHighlight)
        {
            g_debugTexHighlight->Release();
            g_debugTexHighlight = nullptr;
        }
    }

    // -------------------------------------------------------------------------
    // Texture cycler
    // Tracks textures seen each frame in first-bind order
    // Rebuilt every frame: ClearFrameList() in hook_Present, populated in HandleSetTexture
    // Hotkeys (Page Up / Page Down / F9) cycle selection and dump from d3d9_hooks.h
    // -------------------------------------------------------------------------
    inline std::vector<uint64_t> g_frameTextures; // ordered unique stage-0 hashes this frame
    inline std::unordered_set<uint64_t> g_frameSeen; // dedup companion
    inline uint64_t g_selectedHash = 0; // 0 = nothing selected

    // called from hook_Present AFTER reading hotkeys, so the list is still populated when keys are checked
    inline void ClearFrameList()
    {
        g_frameTextures.clear();
        g_frameSeen.clear();
    }

    // -------------------------------------------------------------------------
    // Path helpers
    // -------------------------------------------------------------------------

    inline std::wstring GetModDir()
    {
        wchar_t modulePath[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
        return (fs::path(modulePath).parent_path() / "ssa-improved").wstring();
    }

    inline std::wstring GetTexturesDir() { return (fs::path(GetModDir()) / "textures").wstring(); }
    inline std::wstring GetDumpDir() { return (fs::path(GetModDir()) / "dumps" / "textures").wstring(); }

    // -------------------------------------------------------------------------
    // Helpers
    // -------------------------------------------------------------------------

    /**
     * FNV-1a 64-bit hash
     */
    inline uint64_t FNV1a64(const uint8_t* data, size_t len, uint64_t h = 14695981039346656037ull)
    {
        for (size_t i = 0; i < len; ++i)
            h = (h ^ data[i]) * 1099511628211ull;
        return h;
    }

    /**
     * Get display name for formats (used for dump filenames so modders can identify textures)
     */
    inline const char* FormatName(D3DFORMAT fmt)
    {
        switch (fmt)
        {
            case D3DFMT_DXT1: return "DXT1";
            case D3DFMT_DXT2: return "DXT2";
            case D3DFMT_DXT3: return "DXT3";
            case D3DFMT_DXT4: return "DXT4";
            case D3DFMT_DXT5: return "DXT5";
            case D3DFMT_A8R8G8B8: return "A8R8G8B8";
            case D3DFMT_X8R8G8B8: return "X8R8G8B8";
            case D3DFMT_A8: return "A8";
            case D3DFMT_L8: return "L8";
            case D3DFMT_A4R4G4B4: return "A4R4G4B4";
            default: return "UNK";
        }
    }

    // -------------------------------------------------------------------------
    // Hashing
    // -------------------------------------------------------------------------

    /**
     * Hashes mip 0 of a 2D texture via a read-only lock.
     * @return Hash, or 0 if the texture can't or shouldn't be hashed
     *         (render targets, dynamic textures, lock failures, unknown format)
     */
    inline uint64_t HashTexture(IDirect3DTexture9* pTex)
    {
        D3DSURFACE_DESC desc;
        if (FAILED(pTex->GetLevelDesc(0, &desc)))
            return 0;

        // skip render targets: can't lock with READONLY, and we never want to replace them
        // skip dynamic textures (font atlases, UI sprite sheets): same pointer, different content every frame
        if (desc.Usage & D3DUSAGE_RENDERTARGET) return 0;
        if (desc.Usage & D3DUSAGE_DYNAMIC) return 0;

        D3DLOCKED_RECT lr;
        if (FAILED(pTex->LockRect(0, &lr, nullptr, D3DLOCK_READONLY)))
            return 0;

        size_t rowBytes = 0, numRows = 0;
        if (FAILED(TextureFormat::GetSurfaceInfo(desc.Width, desc.Height, desc.Format, nullptr, &rowBytes, &numRows)))
        {
            pTex->UnlockRect(0);
            return 0;
        }

        uint64_t h = 14695981039346656037ull;
        const auto* pRow = static_cast<const uint8_t*>(lr.pBits);
        for (size_t row = 0; row < numRows; ++row)
        {
            h = FNV1a64(pRow, rowBytes, h);
            pRow += lr.Pitch;
        }

        pTex->UnlockRect(0);
        return h != 0 ? h : 1ull; // avoid returning 0 for a valid (but all-zero) texture
    }

    // -------------------------------------------------------------------------
    // Dumping
    // -------------------------------------------------------------------------

    /**
     * Saves mip 0 of a game texture as DDS once per hash per session<br>
     * Generated name: <code><HASH>_<W>x<H>_<FMT>.dds</code><br>
     * To use as a replacement: rename to <code><HASH>.dds</code> and drop in <code>ssa-improved/textures/</code>
     */
    inline void Dump(IDirect3DTexture9* pTex, uint64_t hash)
    {
        if (g_dumped.count(hash)) return;
        g_dumped.insert(hash);

        D3DSURFACE_DESC desc;
        if (FAILED(pTex->GetLevelDesc(0, &desc))) return;

        // SaveDDSTextureToFile takes IDirect3DSurface9* → pull mip 0 out with GetSurfaceLevel
        IDirect3DSurface9* pSurface = nullptr;
        if (FAILED(pTex->GetSurfaceLevel(0, &pSurface)) || !pSurface) return;

        std::wstring dumpDir = GetDumpDir();
        fs::create_directories(dumpDir);

        wchar_t filename[128];
        swprintf_s(filename, L"%016llX_%ux%u_%hs.dds",
                   hash, desc.Width, desc.Height, FormatName(desc.Format));

        std::wstring outPath = (fs::path(dumpDir) / filename).wstring();

        HRESULT hr = DirectX::SaveDDSTextureToFile(pSurface, outPath.c_str());
        pSurface->Release();

        if (SUCCEEDED(hr))
            LogDebug("[TextureMods] Dumped %016llX (%ux%u %s)",
                     hash, desc.Width, desc.Height, FormatName(desc.Format));
        else
            Log("[TextureMods] Dump failed for %016llX hr=0x%08X", hash, hr);
    }

    // -------------------------------------------------------------------------
    // Cycler navigation
    // -------------------------------------------------------------------------

    inline void LogSelection()
    {
        if (g_selectedHash == 0) return;

        // compute display position from current frame list
        int pos = -1;
        for (int i = 0; i < (int)g_frameTextures.size(); i++)
            if (g_frameTextures[i] == g_selectedHash)
            {
                pos = i;
                break;
            }

        // read from cache, never touch COM pointers here, may be stale
        auto it = g_hashToDesc.find(g_selectedHash);
        if (it != g_hashToDesc.end())
            LogDebug("[Cycler] [%d/%d] hash=%016llX %ux%u %s", pos, (int)g_frameTextures.size() - 1,
                     g_selectedHash, it->second.Width, it->second.Height, FormatName(it->second.Format));
        else
            LogDebug("[Cycler] [%d/%d] hash=%016llX",
                     pos, (int)g_frameTextures.size() - 1, g_selectedHash);
    }

    inline void SelectNext()
    {
        if (g_frameTextures.empty()) return;

        // find current position (-1 if selected hash not in this frame's list)
        int cur = -1;
        for (int i = 0; i < (int)g_frameTextures.size(); i++)
            if (g_frameTextures[i] == g_selectedHash)
            {
                cur = i;
                break;
            }

        // advance forward, skipping excluded hashes
        for (int attempt = 0; attempt < (int)g_frameTextures.size(); attempt++)
        {
            cur = (cur + 1) % (int)g_frameTextures.size();
            g_selectedHash = g_frameTextures[cur];
            LogSelection();
            return;
        }
    }

    inline void SelectPrev()
    {
        if (g_frameTextures.empty()) return;

        // find current position; if not found, start from end so -1 wraps to last entry
        int cur = (int)g_frameTextures.size();
        for (int i = 0; i < (int)g_frameTextures.size(); i++)
            if (g_frameTextures[i] == g_selectedHash)
            {
                cur = i;
                break;
            }

        // advance backward, skipping excluded hashes
        for (int attempt = 0; attempt < (int)g_frameTextures.size(); attempt++)
        {
            cur = (cur - 1 + (int)g_frameTextures.size()) % (int)g_frameTextures.size();
            g_selectedHash = g_frameTextures[cur];
            LogSelection();
            return;
        }
    }

    /**
     * Dumps the currently selected texture, even if already dumped this session.
     * Only proceeds if the texture appeared in the current frame (pointer guaranteed live).
     */
    inline void DumpSelected()
    {
        if (g_selectedHash == 0)
        {
            Log("[Cycler] DumpSelected: nothing selected");
            return;
        }

        // g_frameSeen contains textures bound during the current frame -> pointers are live
        // if the selected texture wasn't rendered this frame, skip
        if (!g_frameSeen.count(g_selectedHash))
        {
            Log("[Cycler] DumpSelected: hash %016llX not in current frame, skipping", g_selectedHash);
            return;
        }

        for (auto& [ptr, h] : g_ptrToHash)
        {
            if (h != g_selectedHash) continue;
            g_dumped.erase(g_selectedHash); // force re-dump even if already dumped this session
            Dump(ptr, g_selectedHash);
            return;
        }
        Log("[Cycler] DumpSelected: no pointer found for hash %016llX", g_selectedHash);
    }

    // -------------------------------------------------------------------------
    // Loading replacements
    // -------------------------------------------------------------------------

    /**
     * Creates debug textures and scans <code>ssa-improved/textures/</code> for replacement DDS files.<br>
     * Called once from hook_Present at startup and again after every device Reset.
     */
    inline void Load(IDirect3DDevice9* pDevice)
    {
        CreateDebugTextures(pDevice);
        g_loaded = true; // always mark initialized, regardless of which features are active

        if (!g_config.textureMods)
            return;

        std::wstring texDir = GetTexturesDir();
        if (!fs::exists(texDir))
        {
            fs::create_directories(texDir);
            return;
        }

        int loaded = 0, failed = 0, skipped = 0;

        for (const auto& entry : fs::recursive_directory_iterator(texDir))
        {
            if (!entry.is_regular_file()) continue;

            const auto& path = entry.path();
            if (_wcsicmp(path.extension().c_str(), L".dds") != 0) continue;

            // filename must be a plain hex hash, nothing else
            std::wstring stem = path.stem().wstring();
            wchar_t* parseEnd = nullptr;
            unsigned long long hashVal = wcstoull(stem.c_str(), &parseEnd, 16);
            if (!parseEnd || *parseEnd != L'\0' || hashVal == 0)
            {
                Log("[TextureMods] Skipping '%ls': not a valid hex hash filename",
                    path.filename().c_str());
                skipped++;
                continue;
            }

            auto hash = static_cast<uint64_t>(hashVal);

            IDirect3DBaseTexture9* pReplacement = nullptr;
            HRESULT hr = DirectX::CreateDDSTextureFromFile(pDevice, path.c_str(), &pReplacement);

            if (SUCCEEDED(hr) && pReplacement)
            {
                g_replacements[hash] = pReplacement;
                LogDebug("[TextureMods] Loaded replacement %016llX", hash);
                loaded++;
            }
            else
            {
                Log("[TextureMods] Failed to load '%ls' hr=0x%08X",
                    path.filename().c_str(), hr);
                failed++;
            }
        }

        Log("[TextureMods] Replacements: %d loaded, %d failed, %d skipped", loaded, failed, skipped);
    }

    // -------------------------------------------------------------------------
    // Reset / Reload
    // -------------------------------------------------------------------------

    /**
     * Clears the pointer → hash cache & triggers reload of all textures by releasing replacements and clearing the hash → replacement map
     */
    inline void OnReset()
    {
        // DDSTextureLoader9 creates textures in D3DPOOL_DEFAULT -> must be released before Reset
        // hook_Present will call Load() again to recreate them
        for (auto& [hash, pTex] : g_replacements)
            if (pTex) pTex->Release();
        g_replacements.clear();

        g_loaded = false;
        g_reloadPending = false;

        g_ptrToHash.clear();
        g_frameTextures.clear();
        g_frameSeen.clear();
        // g_selectedHash intentionally preserved across resets

        Log("[TextureMods] Released DEFAULT pool replacements on Reset");
    }

    /**
     * Hot Reload<br>
     * Useful for mod authors to see changes without restarting the game
     * @param pDevice
     */
    inline void Reload(IDirect3DDevice9* pDevice)
    {
        for (auto& [hash, pTex] : g_replacements)
            if (pTex) pTex->Release();
        g_replacements.clear();
        // g_ptrToHash intentionally preserved, game texture hashes don't change
        Load(pDevice);
    }

    // -------------------------------------------------------------------------
    // Hook Execution: SetTexture
    // -------------------------------------------------------------------------

    inline IDirect3DBaseTexture9* HandleSetTexture(IDirect3DDevice9* pDevice, DWORD Stage, IDirect3DBaseTexture9* pTex)
    {
        // only handle plain 2D textures, cube and volume textures pass through
        if (pTex->GetType() != D3DRTYPE_TEXTURE)
            return pTex;

        auto* pTex2D = static_cast<IDirect3DTexture9*>(pTex);

        // hash on first encounter, emplace(ptr, 0) inserts only if the pointer isn't already known, so HashTexture is called exactly once
        auto [it, inserted] = g_ptrToHash.emplace(pTex2D, 0u);
        if (inserted)
        {
            it->second = HashTexture(pTex2D);
            // cache desc at hash time so LogSelection() never needs to touch a COM pointer later
            if (it->second != 0)
            {
                D3DSURFACE_DESC desc;
                if (SUCCEEDED(pTex2D->GetLevelDesc(0, &desc)))
                    g_hashToDesc[it->second] = desc;
            }
        }

        const uint64_t hash = it->second;

        if (hash != 0)
        {
            if (g_config.textureDump)
                Dump(pTex2D, hash);

            if (g_config.textureMods)
            {
                auto repl = g_replacements.find(hash);
                if (repl != g_replacements.end())
                    pTex = repl->second;
            }

            // cycler: track textures and highlight the selected one
            if (g_config.textureCycler)
            {
                if (g_frameSeen.insert(hash).second)
                    g_frameTextures.push_back(hash);

                // hash-based: highlights the texture regardless of where it appears in the list
                if (g_selectedHash != 0 && hash == g_selectedHash && g_debugTexHighlight)
                    pTex = g_debugTexHighlight;
            }
        }

        return pTex;
    }
} // namespace ssa::TextureMods
