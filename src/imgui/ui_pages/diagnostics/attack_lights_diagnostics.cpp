#include <imgui.h>
#include "config.h"
#include "game/custom/attack_lights.h"
#include "game/data/attack_light_profiles.h"
#include "imgui/ui.h"
#include "imgui/ui_pages/diagnostics/debug_table.h"

namespace ssa::UIPages
{
    namespace
    {
        void RenderProfiles()
        {
            DebugTable table{"Name", "Character", "Trigger tag", "Colour", "Intensity", "Radius"};
            for (const auto& profile : Game::Data::kAttackLightProfiles)
            {
                table.AddRow()
                     .Add(profile.name)
                     .Add(Hex(profile.key.character))
                     .Add(Hex(profile.key.triggerTag))
                     .Add(Fmt("%.2f %.2f %.2f", profile.colour[0], profile.colour[1], profile.colour[2]))
                     .Add(Fmt("%.1fx", profile.intensity))
                     .Add(Fmt("%.1f", profile.radius));
            }
            table.Show("##attack_light_profiles");

            ImGui::TextDisabled(
                "A trigger tag of 0x00000000 matches the projectiles of that character without a trigger script. Different attacks can share it.");
        }

        void RenderDiscovery()
        {
            ImGui::TextWrapped(
                "Record while firing one attack at a time. Projectiles are grouped by character and trigger tag, both local players are supported. "
                "Recording does not enable any lights.");

            static bool recording = false;
            ImGui::Checkbox("Record projectiles", &recording);
            if (recording)
                Game::AttackLights::KeepRecording(); // stops by itself shortly after this panel is hidden
            ImGui::SameLine();
            if (ImGui::Button("Clear observations"))
                Game::AttackLights::ClearObservations();

            DebugTable table{"Character", "CRC", "Trigger tag", "Live", "Native light", "Profile"};
            for (const auto& observation : Game::AttackLights::GetObservations())
            {
                DebugTable::Row& row = table.AddRow();
                const auto* profile = Game::Data::FindAttackLightProfile(observation.key);
                if (profile)
                    row.background = TableColor::RowHighlight;

                row.Add(observation.characterName)
                   .Add(Hex(observation.key.character))
                   .Add(Hex(observation.key.triggerTag))
                   .Add(Fmt("%zu", observation.live));
                if (observation.nativeLight) row.Add("yes", TableColor::Amber);
                else row.Dim("no");
                if (profile) row.Add(profile->name, TableColor::Green);
                else row.Dim("-");
            }
            table.Show("##attack_light_observations");
        }

        void RenderTestLight()
        {
            if (ImGui::Button("Test light on player 1"))
                Game::AttackLights::SpawnTestLight();
            ImGui::SameLine();
            UI::HelpMarker("Puts a light on player 1 for three seconds.");

            auto& settings = Game::AttackLights::GetTestLightSettings();
            ImGui::SliderFloat("Test intensity", &settings.intensity, 0.1f, 10.0f, "%.1fx",
                               ImGuiSliderFlags_AlwaysClamp);
            ImGui::SliderFloat("Test radius", &settings.radius, 1.0f, 24.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
        }
    } // namespace

    void RenderAttackLightsDiagnostics()
    {
        if (!ImGui::TreeNode("Attack Lights")) return;
        ImGui::Indent();

        const auto stats = Game::AttackLights::GetStats();
        ImGui::TextDisabled("Enabled: %s (Settings > Graphics)", g_config.attackLights ? "yes" : "no");
        ImGui::TextDisabled("Matched projectiles: %zu", stats.matched);
        ImGui::TextDisabled("Lights: %zu / %zu", stats.active, stats.maxLights);
        ImGui::TextDisabled("Created: %u", stats.created);

        const std::string& status = Game::AttackLights::GetStatus();
        if (!status.empty())
            ImGui::TextWrapped("%s", status.c_str());

        if (ImGui::TreeNode("Profiles"))
        {
            ImGui::Indent();
            RenderProfiles();
            ImGui::Unindent();
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Discover projectile attacks"))
        {
            ImGui::Indent();
            RenderDiscovery();
            ImGui::Unindent();
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Test light"))
        {
            ImGui::Indent();
            RenderTestLight();
            ImGui::Unindent();
            ImGui::TreePop();
        }

        ImGui::Unindent();
        ImGui::TreePop();
    }
} // namespace ssa::UIPages
