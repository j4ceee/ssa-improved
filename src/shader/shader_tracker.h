#pragma once

#include <Windows.h>
#include <d3d9.h>
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include "d3dx_runtime.h"
#include "dump_util.h"
#include "shader_tweaks.h"
#include "shader_patches.h"
#include "shader/ui_hider.h"
#include "log.h"
#include "window/render_targets.h"
#include "window/texture_mods.h"

// Pixel shader inventory for the render target window.
// - Every CreatePixelShader call is hashed (FNV-1a over bytecode) & bytecode is kept, so shaders can be dumped later.
// - While the Render Targets window is open, every draw call is attributed to the pixel shader that is set,
//   the render target bound in slot 0 (the one it writes) and the render-target textures bound to the sampler stages (the ones it reads).
// - "Dump all shaders" writes ps_<hash>.bin (raw bytecode) and ps_<hash>.txt (disassembly with the constant table, which contains the shader's parameter names).
namespace ssa::ShaderTracker
{
    inline constexpr uint64_t kNoShader = 0; // fixed-function pipeline (no pixel shader set)
    inline constexpr uint64_t kUnknownShader = ~0ull; // a shader we never saw being created, or could not parse

    struct ShaderInfo
    {
        uint64_t hash = 0;
        std::vector<DWORD> code; // empty for fixed-function / unknown

        uint32_t draws = 0, drawsLast = 0; // current / previous frame
        uint64_t totalDraws = 0;
        std::unordered_map<const void*, uint32_t> writes; // render target key -> draw count (slot 0)
        std::unordered_set<const void*> reads; // render target keys sampled by this shader
    };

    // all guarded by RenderTargets::g_mutex
    inline std::unordered_map<uint64_t, ShaderInfo> g_shaders;
    inline std::unordered_map<const void*, uint64_t> g_shaderHash; // live shader object -> hash

    // current pixel shader, updated lock-free by hook_SetPixelShader
    inline std::atomic<const void*> g_curPS{nullptr};

    // shaders whose draw calls are skipped (debug aid, see ShouldSkipDraw); guarded by RenderTargets::g_mutex
    inline std::unordered_set<uint64_t> g_hidden;
    inline std::atomic<bool> g_anyHidden{false};

    // -------------------------------------------------------------------------
    // bytecode helpers
    // -------------------------------------------------------------------------

    // number of DWORDs in a shader model 2.0+ pixel shader (including the END token), 0 if it can't be parsed
    inline size_t TokenCount(const DWORD* fn)
    {
        if (!fn) return 0;
        const DWORD version = fn[0];
        if ((version & 0xFFFF0000) != 0xFFFF0000) return 0; // not a pixel shader
        if (((version >> 8) & 0xFF) < 2) return 0; // ps_1_x does not encode instruction lengths, skip

        constexpr size_t kMaxTokens = 16384;
        size_t n = 1;
        while (n < kMaxTokens)
        {
            const DWORD t = fn[n];
            const DWORD op = t & 0xFFFF;
            if (op == 0xFFFF) return n + 1; // D3DSIO_END
            if (op == 0xFFFE) // D3DSIO_COMMENT: length in bits 16..30
                n += 1 + ((t >> 16) & 0x7FFF);
            else // instruction: operand count in bits 24..27
                n += 1 + ((t >> 24) & 0x0F);
        }
        return 0;
    }

    inline std::string HashText(uint64_t h)
    {
        if (h == kNoShader) return "fixed-function";
        if (h == kUnknownShader) return "unknown";
        char buf[24];
        snprintf(buf, sizeof(buf), "%016llX", static_cast<unsigned long long>(h));
        return buf;
    }

    // short form for the render target table (the file names use the full 16 digits, this is their prefix)
    inline std::string ShortHashText(uint64_t h)
    {
        if (h == kNoShader) return "FF";
        if (h == kUnknownShader) return "?";
        char buf[16];
        snprintf(buf, sizeof(buf), "%08X", static_cast<uint32_t>(h >> 32));
        return buf;
    }

    // -------------------------------------------------------------------------
    // hook entry points
    // -------------------------------------------------------------------------

    // called after every successful CreatePixelShader
    inline void OnPixelShaderCreated(IDirect3DPixelShader9* shader, const DWORD* fn)
    {
        if (!shader) return;

        uint64_t h = kUnknownShader;
        const size_t n = TokenCount(fn);
        if (n)
        {
            h = TextureMods::FNV1a64(reinterpret_cast<const uint8_t*>(fn), n * sizeof(DWORD));
            if (h == kNoShader || h == kUnknownShader) h = 2; // keep the two special values reserved
        }

        // other modules only need the hash and bytecode, not the tracker's lock
        ShaderTweaks::OnShaderCreated(shader, fn, n);
        ShaderPatches::OnShaderCreated(shader, h, fn, n);
        UiHider::OnShaderCreated(shader, h);

        std::lock_guard<std::mutex> lk(RenderTargets::g_mutex);
        g_shaderHash[shader] = h; // overwrites a stale entry if the address was reused
        if (h != kUnknownShader)
        {
            auto [it, inserted] = g_shaders.try_emplace(h);
            if (inserted)
            {
                it->second.hash = h;
                it->second.code.assign(fn, fn + n);
            }
        }
    }

    // called from every draw hook (only while RenderTargets::g_tracking)
    inline void OnDraw()
    {
        std::lock_guard<std::mutex> lk(RenderTargets::g_mutex);

        uint64_t hash = kNoShader;
        if (const void* ps = g_curPS.load(std::memory_order_relaxed))
        {
            auto it = g_shaderHash.find(ps);
            hash = (it != g_shaderHash.end()) ? it->second : kUnknownShader;
        }

        ShaderInfo& s = g_shaders[hash];
        s.hash = hash;
        ++s.draws;
        ++s.totalDraws;

        if (const void* out = RenderTargets::g_boundKey)
            ++s.writes[out];

        for (auto& slot : RenderTargets::g_stageKey)
        {
            const void* k = slot.load(std::memory_order_relaxed);
            if (k && RenderTargets::g_index.count(k))
                s.reads.insert(k);
        }
    }

    // called once per frame from the main-thread Present
    inline void OnPresent()
    {
        if (!RenderTargets::g_tracking) return;

        std::lock_guard<std::mutex> lk(RenderTargets::g_mutex);
        for (auto& [h, s] : g_shaders)
        {
            s.drawsLast = s.draws;
            s.draws = 0;
        }
    }

    // called from hook_Reset BEFORE orig_Reset: render target keys become meaningless, shaders themselves survive
    inline void OnReset()
    {
        std::lock_guard<std::mutex> lk(RenderTargets::g_mutex);
        for (auto& [h, s] : g_shaders)
        {
            s.writes.clear();
            s.reads.clear();
        }
    }

    inline void ResetCounters()
    {
        std::lock_guard<std::mutex> lk(RenderTargets::g_mutex);
        for (auto& [h, s] : g_shaders)
        {
            s.draws = s.drawsLast = 0;
            s.totalDraws = 0;
            s.writes.clear();
            s.reads.clear();
        }
    }

    // -------------------------------------------------------------------------
    // hiding shaders (debug aid): draw calls made with a hidden pixel shader are simply not executed
    // -------------------------------------------------------------------------

    inline void SetHidden(uint64_t hash, bool hide)
    {
        // fixed-function draws include ImGui itself, and unknown shaders can't be told apart
        if (hash == kNoShader || hash == kUnknownShader) return;

        std::lock_guard<std::mutex> lk(RenderTargets::g_mutex);
        if (hide) g_hidden.insert(hash);
        else g_hidden.erase(hash);
        g_anyHidden.store(!g_hidden.empty(), std::memory_order_relaxed);
    }

    inline void ShowAll()
    {
        std::lock_guard<std::mutex> lk(RenderTargets::g_mutex);
        g_hidden.clear();
        g_anyHidden.store(false, std::memory_order_relaxed);
    }

    inline size_t HiddenCount()
    {
        std::lock_guard<std::mutex> lk(RenderTargets::g_mutex);
        return g_hidden.size();
    }

    // called first in every draw hook
    inline bool ShouldSkipDraw()
    {
        if (!g_anyHidden.load(std::memory_order_relaxed)) return false;

        const void* ps = g_curPS.load(std::memory_order_relaxed);
        if (!ps) return false;

        std::lock_guard<std::mutex> lk(RenderTargets::g_mutex);
        auto it = g_shaderHash.find(ps);
        return it != g_shaderHash.end() && g_hidden.count(it->second) != 0;
    }

    // -------------------------------------------------------------------------
    // UI side
    // -------------------------------------------------------------------------
    struct ShaderRow
    {
        bool hidden = false;
        uint64_t hash = 0;
        uint32_t sizeBytes = 0;
        uint32_t drawsLast = 0;
        uint64_t totalDraws = 0;
        std::vector<const void*> writes;
        std::vector<const void*> reads;
    };

    inline std::vector<ShaderRow> Snapshot()
    {
        std::lock_guard<std::mutex> lk(RenderTargets::g_mutex);

        std::vector<ShaderRow> out;
        out.reserve(g_shaders.size());
        for (const auto& [h, s] : g_shaders)
        {
            ShaderRow r;
            r.hidden = g_hidden.count(h) != 0;
            r.hash = h;
            r.sizeBytes = static_cast<uint32_t>(s.code.size() * sizeof(DWORD));
            r.drawsLast = s.drawsLast;
            r.totalDraws = s.totalDraws;

            std::vector<std::pair<const void*, uint32_t>> w(s.writes.begin(), s.writes.end());
            std::sort(w.begin(), w.end(), [](const auto& a, const auto& b) { return a.second > b.second; });
            for (const auto& p : w) r.writes.push_back(p.first);

            r.reads.assign(s.reads.begin(), s.reads.end());
            out.push_back(std::move(r));
        }

        std::sort(out.begin(), out.end(), [](const ShaderRow& a, const ShaderRow& b) { return a.hash < b.hash; });
        return out;
    }

    // render target pointer -> index in the Render Targets table
    inline std::unordered_map<const void*, size_t> IndexByPtr(const std::vector<RenderTargets::Entry>& rows)
    {
        std::unordered_map<const void*, size_t> m;
        for (size_t i = 0; i < rows.size(); ++i)
            if (!rows[i].released)
                m[rows[i].ptr] = i;
        return m;
    }

    inline std::string TargetList(const std::vector<const void*>& keys, const std::unordered_map<const void*, size_t>& idx)
    {
        std::vector<size_t> ids;
        for (const void* k : keys)
            if (auto it = idx.find(k); it != idx.end())
                ids.push_back(it->second);
        std::sort(ids.begin(), ids.end());

        std::string s;
        for (size_t id : ids)
        {
            if (!s.empty()) s += ',';
            s += '#';
            s += std::to_string(id);
        }
        return s.empty() ? "-" : s;
    }

    // -------------------------------------------------------------------------
    // dumping
    // -------------------------------------------------------------------------
    inline std::string DescribeTargets(const std::vector<const void*>& keys, const std::unordered_map<const void*, size_t>& idx,
        const std::vector<RenderTargets::Entry>& rows)
    {
        std::string out;
        char buf[128];
        for (const void* k : keys)
        {
            auto it = idx.find(k);
            if (it == idx.end()) continue;
            const auto& e = rows[it->second];
            snprintf(buf, sizeof(buf), "//   #%zu  %ux%u %s\n", it->second, e.w, e.h,
                     RenderTargets::FormatName(e.format).c_str());
            out += buf;
        }
        return out.empty() ? "//   (none)\n" : out;
    }

    // returns the number of shaders written
    inline int DumpAll(const std::vector<RenderTargets::Entry>& rows, const std::vector<ShaderRow>& shaderRows)
    {
        // copy bytecode out under the lock, do the slow file work without it
        std::unordered_map<uint64_t, std::vector<DWORD>> codes;
        {
            std::lock_guard<std::mutex> lk(RenderTargets::g_mutex);
            for (const auto& [h, s] : g_shaders)
                if (!s.code.empty())
                    codes.emplace(h, s.code);
        }

        const auto dir = DumpUtil::ShaderDumpDir();
        const auto disassemble = D3DXRuntime::GetFunction<
            D3DXRuntime::D3DXDisassembleShader_t>("D3DXDisassembleShader");
        const auto idx = IndexByPtr(rows);

        std::unordered_map<uint64_t, const ShaderRow*> rowByHash;
        for (const auto& r : shaderRows) rowByHash[r.hash] = &r;

        for (const auto& [h, code] : codes)
        {
            char base[40];
            snprintf(base, sizeof(base), "ps_%016llX", static_cast<unsigned long long>(h));

            // raw bytecode
            if (DumpUtil::OutFile bin{dir / (std::string(base) + ".bin"), true})
                bin.Write(code.data(), sizeof(DWORD), code.size());

            // disassembly + what we know about how the game uses it
            if (DumpUtil::OutFile txt{dir / (std::string(base) + ".txt")})
            {
                txt.Printf("// ssa-improved pixel shader dump\n// hash: %016llX   size: %zu bytes\n",
                           static_cast<unsigned long long>(h), code.size() * sizeof(DWORD));

                if (auto it = rowByHash.find(h); it != rowByHash.end())
                {
                    const ShaderRow& r = *it->second;
                    txt.Printf("// draws observed: %llu\n", static_cast<unsigned long long>(r.totalDraws));
                    txt.Printf("// renders into:\n%s", DescribeTargets(r.writes, idx, rows).c_str());
                    txt.Printf("// samples these render targets:\n%s", DescribeTargets(r.reads, idx, rows).c_str());
                }
                txt.Puts("\n");

                D3DXRuntime::BlobPtr text;
                if (!disassemble)
                    txt.Puts("// D3DXDisassembleShader not available, only the .bin was written\n");
                else if (SUCCEEDED(disassemble(code.data(), FALSE, nullptr, text.Receive())) && text)
                    txt.Puts(text.Text());
                else
                    txt.Puts("// D3DXDisassembleShader failed\n");
            }
        }

        Log("[ShaderTracker] Dumped %zu pixel shaders to %ls", codes.size(), dir.c_str());
        return static_cast<int>(codes.size());
    }
} // namespace ssa::ShaderTracker
