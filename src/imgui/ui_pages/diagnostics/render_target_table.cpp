#include <cstdint>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

#include <imgui.h>
#include "imgui/ui.h"
#include "imgui/ui_pages/diagnostics/debug_table.h"
#include "window/render_targets.h"
#include "shader/shader_tracker.h"

namespace ssa::UIPages
{
    namespace
    {
        using RenderTargets::Entry;
        using RenderTargets::Kind;
        using ShaderTracker::ShaderRow;

        // render target pointer -> shaders that drew into it (most-used shader first)
        using WriterMap = std::unordered_map<const void*, std::vector<uint64_t>>;

        WriterMap BuildWriters(const std::vector<ShaderRow>& shaders)
        {
            WriterMap m;
            for (const auto& s : shaders)
                for (const void* k : s.writes)
                    m[k].push_back(s.hash);
            return m;
        }

        std::string WritersText(const Entry& e, const WriterMap& writers, size_t maxShown)
        {
            auto it = writers.find(e.ptr);
            if (it == writers.end() || e.released) return "-";

            std::string out;
            size_t shown = 0;
            for (uint64_t h : it->second)
            {
                if (shown == maxShown) break;
                if (!out.empty()) out += ", ";
                out += ShaderTracker::ShortHashText(h);
                ++shown;
            }
            if (it->second.size() > shown)
                out += " +" + std::to_string(it->second.size() - shown);
            return out;
        }

        const char* KindName(Kind k)
        {
            switch (k)
            {
            case Kind::Texture: return "texture";
            case Kind::Surface: return "surface";
            case Kind::DepthSurface: return "depth surface";
            }
            return "?";
        }

        std::string SizeText(const Entry& e)
        {
            if (e.w != e.reqW || e.h != e.reqH)
                return Fmt("%ux%u (game: %ux%u)", e.w, e.h, e.reqW, e.reqH);
            return Fmt("%ux%u", e.w, e.h);
        }

        DebugTable BuildRenderTargetTable(const std::vector<Entry>& rows, const WriterMap& writers)
        {
            DebugTable table{
                "#", "Kind", "Size", "Format", "Usage", "Binds/frame", "Samples/frame", "Status", "Drawn by"
            };
            for (size_t i = 0; i < rows.size(); ++i)
            {
                const Entry& e = rows[i];
                DebugTable::Row& row = table.AddRow();
                row.Add(std::to_string(i))
                   .Add(KindName(e.kind))
                   .Add(SizeText(e))
                   .Add(RenderTargets::FormatName(e.format))
                   .Add(RenderTargets::UsageString(e.usage))
                   .Add(std::to_string(e.bindsLast))
                   .Add(std::to_string(e.samplesLast));

                const char* status = RenderTargets::StatusText(e);
                if (std::strcmp(status, "live") == 0) row.Add(status, TableColor::Green);
                else if (std::strcmp(status, "write only") == 0) row.Add(status, TableColor::Amber);
                else if (e.released || std::strcmp(status, "unused") == 0) row.Dim(status);
                else row.Add(status);

                // the table shows the first three shaders, hovering / copying gives all of them
                std::string shown = WritersText(e, writers, 3);
                std::string all = WritersText(e, writers, 99);
                row.AddFull(shown, all == shown ? std::string() : all);
            }
            return table;
        }

        void RenderTargetsTab(const std::vector<Entry>& rows, const WriterMap& writers)
        {
            const DebugTable table = BuildRenderTargetTable(rows, writers);
            table.CopyButton();
            ImGui::Spacing();

            if (rows.empty())
            {
                ImGui::TextDisabled("Nothing recorded yet.");
                return;
            }
            table.Draw("##rt_list", ImGuiTableFlags_ScrollY);
        }

        void RenderShadersTab(const std::vector<Entry>& rows, const std::vector<ShaderRow>& shaders)
        {
            static bool s_onlyUsed = true;
            static int s_lastDumpCount = -1;

            const auto idx = ShaderTracker::IndexByPtr(rows);

            // what you see is what the copy button copies
            std::vector<const ShaderRow*> shown;
            DebugTable table{"Hash", "Bytes", "Draws/frame", "Total draws", "Writes", "Reads"};
            for (const auto& s : shaders)
            {
                if (s_onlyUsed && s.totalDraws == 0 && !s.hidden) continue;
                shown.push_back(&s);

                DebugTable::Row& row = table.AddRow();
                row.Add(ShaderTracker::HashText(s.hash), s.hidden ? TableColor::Amber : 0)
                   .Add(std::to_string(s.sizeBytes))
                   .Add(std::to_string(s.drawsLast))
                   .Add(std::to_string(s.totalDraws))
                   .Add(ShaderTracker::TargetList(s.writes, idx))
                   .Add(ShaderTracker::TargetList(s.reads, idx));
            }

            if (ImGui::Button("Dump all shaders"))
                s_lastDumpCount = ShaderTracker::DumpAll(rows, shaders);
            ImGui::SameLine();
            table.CopyButton("Copy shader table to clipboard");
            ImGui::SameLine();
            UI::HelpMarker("Dump writes ps_<hash>.bin (bytecode) and ps_<hash>.txt (disassembly, including the shader's constant names) "
                "to ssa-improved/dumps/shaders/ in the game folder.");

            if (s_lastDumpCount >= 0)
                ImGui::TextDisabled("Dumped %d shaders to ssa-improved/dumps/shaders/", s_lastDumpCount);

            // hiding shaders: their draw calls are skipped, which shows what they are responsible for
            const size_t hiddenCount = ShaderTracker::HiddenCount();
            if (hiddenCount > 0)
            {
                UI::WarningText(("Hidden shaders: " + std::to_string(hiddenCount)).c_str());
                ImGui::SameLine();
                if (ImGui::Button("Show all"))
                    ShaderTracker::ShowAll();
            }
            else
            {
                ImGui::TextDisabled("Tick 'Hide' on a shader to skip its draw calls and see what it is responsible for.");
            }

            ImGui::Checkbox("Only list shaders that have drawn something", &s_onlyUsed);
            ImGui::TextDisabled("'Writes' = render targets this shader draws into, 'Reads' = render target textures it samples. "
                "Numbers refer to the # column of the Render targets tab.");
            ImGui::Spacing();

            const DebugTable::ExtraColumn hideColumn{
                "Hide", [&](size_t i)
                {
                    const ShaderRow& s = *shown[i];

                    // fixed-function draws include ImGui itself and unknown shaders can't be told apart: never hideable
                    const bool canHide = s.hash != ShaderTracker::kNoShader && s.hash != ShaderTracker::kUnknownShader;
                    ImGui::BeginDisabled(!canHide);
                    ImGui::PushID(static_cast<int>(s.hash ^ (s.hash >> 32)));
                    bool hide = s.hidden;
                    if (ImGui::Checkbox("##hide", &hide))
                        ShaderTracker::SetHidden(s.hash, hide);
                    ImGui::PopID();
                    ImGui::EndDisabled();
                }
            };
            table.Draw("##ps_list", ImGuiTableFlags_ScrollY, &hideColumn);
        }
    } // namespace

    void RenderRenderTargetTable()
    {
        const std::vector<Entry> rows = RenderTargets::Snapshot();
        const std::vector<ShaderRow> shaders = ShaderTracker::Snapshot();
        const WriterMap writers = BuildWriters(shaders);

        ImGui::TextDisabled("Counters only run while this window is open.");
        ImGui::TextDisabled("%zu render targets / depth surfaces, %zu pixel shaders known", rows.size(), shaders.size());
        ImGui::Spacing();

        if (ImGui::Button("Reset counters"))
        {
            RenderTargets::ResetCounters();
            ShaderTracker::ResetCounters();
        }
        ImGui::Spacing();

        if (ImGui::BeginTabBar("##rt_tabs"))
        {
            if (ImGui::BeginTabItem("Render targets"))
            {
                RenderTargetsTab(rows, writers);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Pixel shaders"))
            {
                RenderShadersTab(rows, shaders);
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }
} // namespace ssa::UIPages
