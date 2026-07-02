#include "imgui/ui.h"
#include "imgui/fonts/IconsMaterialDesign.h"
#include <imgui.h>
#include "config.h"
#include "game/character.h"
#include "game/game.h"
#include "imgui/fonts/IconsSkylanders.h"

namespace ssa::UIPages
{
    void RenderGameTab()
    {
        // Difficulty
        // -----------------------------------------------------------------------------------------------------
        if (ImGui::CollapsingHeader(ICON_MD_SPEED " Difficulty", ImGuiTreeNodeFlags_DefaultOpen))
        {
            // HP and damage are snapshot-based - disable until BuildSettings has populated the arrays
            ImGui::SliderFloat("Enemy HP multiplier", &g_config.hpMult, 0.1f, 10.0f, "%.2fx");
            if (ImGui::IsItemDeactivatedAfterEdit())
                SetEnemyHpMultiplier(g_config.hpMult);
            ImGui::SameLine();
            UI::HelpMarker("Scales enemy HP. This multiplier is applied to the base HP values of the enemies in the current level.");

            ImGui::SliderFloat("Enemy damage multiplier", &g_config.dmgMult, 0.1f, 10.0f, "%.2fx");
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                SetEnemyDmgMultiplier(g_config.dmgMult);
            }
            ImGui::SameLine();
            UI::HelpMarker("Scales enemy damage. This multiplier is applied to the base damage values of the enemies in the current level.");

            ImGui::Spacing();
            ImGui::Spacing();

            if (ImGui::Checkbox("Enemy hit reaction", &g_config.enemyHitReaction))
            {
                SetEnemyHitReaction(g_config.enemyHitReaction);
            }
            ImGui::SameLine();
            UI::HelpMarker("Enable or disable enemy hit reaction. When enabled, enemies will react to being hit by the player. "
                "When disabled they will still take damage but will not react to being hit, they will not be stunned and attack faster.");

            ImGui::Spacing();
            ImGui::Spacing();

            ImGui::SliderFloat("Heroic Challenge HP ceiling", &g_config.heroicHpCeiling, 0.1f, 10.0f, "%.2fx");
            if (ImGui::IsItemDeactivatedAfterEdit())
                SetEnemyHpHeroicCeiling(g_config.heroicHpCeiling);
            ImGui::SameLine();
            UI::HelpMarker("Maximum HP enemies can have in Heroic Challenges. If you find that enemies in challenges are too weak, try increasing this.");

            ImGui::SliderFloat("Heroic Challenge damage ceiling", &g_config.heroicDmgCeiling, 0.1f, 10.0f, "%.2fx");
            if (ImGui::IsItemDeactivatedAfterEdit())
                SetEnemyDmgHeroicCeiling(g_config.heroicDmgCeiling);
            ImGui::SameLine();
            UI::HelpMarker("Maximum damage enemies can deal in Heroic Challenges. If you find that enemies in challenges are too weak, try increasing this.");
        }

        ImGui::Spacing();
        ImGui::Spacing();

        // Experience
        // -----------------------------------------------------------------------------------------------------
        if (ImGui::CollapsingHeader(ICON_MD_MILITARY_TECH " Experience", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::SliderFloat("XP multiplier", &g_config.xpMult, 0.1f, 10.0f, "%.2fx");
            if (ImGui::IsItemDeactivatedAfterEdit())
                SetXpMultiplier(g_config.xpMult);
            ImGui::SameLine();
            UI::HelpMarker("Multiplies the amount of XP gained from defeating enemies");
        }

        ImGui::Spacing();
        ImGui::Spacing();

        // Skylanders
        // -----------------------------------------------------------------------------------------------------
        if (ImGui::CollapsingHeader(ICON_SKY_MAGIC " Skylanders", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto* list = Game::Character::instanceSkylandersList();

            if (!list || list->empty())
            {
                ImGui::TextDisabled("No Skylanders on portal.");
            }
            else
            {
                for (const auto& ref : *list)
                {
                    auto* ch = ref.mPtr;
                    if (!ch) continue;

                    ImGui::PushID(static_cast<int>(reinterpret_cast<uintptr_t>(ch)));
                    ImGui::SeparatorText(ch->contID == 0 ? "Player 1" : "Player 2");

                    // Health slider - live write to m_fCurrHealth
                    ImGui::SliderFloat("HP", &ch->m_fCurrHealth, 0.0f, ch->maxHP(), "%.0f");
                    ImGui::SameLine();
                    UI::HelpMarker("Current health. Drag to set HP directly.");

                    ImGui::Spacing();

                    // God mode
                    bool godMode = ch->hasGodMode();
                    if (ImGui::Checkbox("God mode", &godMode))
                        ch->setGodMode(godMode);
                    ImGui::SameLine();
                    UI::HelpMarker("Makes the Skylander immune to all damage.");

                    // No knockback
                    bool noKnockback = ch->hasIgnoreKnockback();
                    if (ImGui::Checkbox("No knockback", &noKnockback))
                        ch->setIgnoreKnockback(noKnockback);
                    ImGui::SameLine();
                    UI::HelpMarker("Prevents the Skylander from being knocked back by attacks.");

                    // No hit reaction
                    bool noHitReaction = ch->hasIgnoreHitReaction();
                    if (ImGui::Checkbox("No hit reaction", &noHitReaction))
                        ch->setIgnoreHitReaction(noHitReaction);
                    ImGui::SameLine();
                    UI::HelpMarker("Prevents attack animations from being interrupted by hits.");

                    // Team dropdown
                    ImGui::Spacing();
                    static constexpr const char* kTeamLabels[] = { "Skylander", "Enemy", "Neutral" };
                    static constexpr Game::CharacterTeam kTeamValues[] = {
                        Game::CharacterTeam::Skylander,
                        Game::CharacterTeam::Enemy,
                        Game::CharacterTeam::Neutral,
                    };
                    int teamIdx = 0;
                    for (int t = 0; t < 3; t++)
                        if (ch->m_team == kTeamValues[t]) { teamIdx = t; break; }
                    if (ImGui::Combo("Team", &teamIdx, kTeamLabels, 3))
                        ch->m_team = kTeamValues[teamIdx];
                    ImGui::SameLine();
                    UI::HelpMarker("Experimental. Changing team may enable friendly fire - effect depends on targeting setup at spawn.");

                    ImGui::PopID();
                    ImGui::Spacing();
                }
            }
        }

        ImGui::Spacing();
        ImGui::Spacing();

        // Level
        // -----------------------------------------------------------------------------------------------------
        if (ImGui::CollapsingHeader(ICON_MD_LANDSCAPE " Level", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto* game = Game::Game::instance();

            if (auto* currentDesc = game->getCurrLevelDesc())
            {
                if (const auto* name = Game::Data::GetLevelDisplayInfo(currentDesc->m_Id))
                {
                    ImGui::TextDisabled("Current Level: %s", name->displayName);
                }
                else
                {
                    ImGui::TextDisabled("Current Level: %s", currentDesc->m_LevelFile.c_str());
                }

                if (std::strcmp(currentDesc->m_Category.c_str(), "PvP_Level") == 0)
                    ImGui::BeginDisabled();

                // --- Level selector
                static uint32_t s_selectedCrc = 0;
                static uint32_t s_lastSeenLevel = 0;
                if (s_selectedCrc == 0 && (std::strcmp(currentDesc->m_LevelFile.c_str(), "FrontEnd") != 0))
                {
                    s_selectedCrc = game->m_CurrLevel; // pre-select current level on first open
                }
                if (game->m_CurrLevel != s_lastSeenLevel)
                {
                    s_selectedCrc = 0;
                    s_lastSeenLevel = game->m_CurrLevel;
                }

                // returns the best available display label for a descriptor
                auto levelLabel = [](const Game::LevelDesc& desc) -> const char*
                {
                    if (const auto* info = Game::Data::GetLevelDisplayInfo(desc.m_Id))
                        return info->displayName;
                    return desc.m_LevelFile.c_str();
                };

                // find preview string for the selected CRC
                const char* preview = "Select a level...";
                for (const auto& desc : game->m_GamePackage.m_Levels)
                    if (desc.m_Id == s_selectedCrc) { preview = levelLabel(desc); break; }

                if (ImGui::BeginCombo("##levelSelect", preview))
                {
                    for (const auto& desc : game->m_GamePackage.m_Levels)
                    {
                        if (std::strcmp(desc.m_Category.c_str(), "PvP_Level") == 0 ||
                            std::strcmp(desc.m_Category.c_str(), "TestLevels") == 0 ||
                            std::strcmp(desc.m_LevelFile.c_str(), "FrontEnd") == 0)
                            continue;

                        const bool isCurrent = desc.m_Id == game->m_CurrLevel;
                        const bool isSelected = desc.m_Id == s_selectedCrc;

                        if (isCurrent)
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.9f, 0.5f, 1.0f));

                        if (ImGui::Selectable(levelLabel(desc), isSelected))
                            s_selectedCrc = desc.m_Id;

                        if (isCurrent) ImGui::PopStyleColor();
                        if (isSelected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                ImGui::SameLine();

                ImGui::BeginDisabled(s_selectedCrc == 0);
                if (ImGui::Button("Load"))
                {
                    game->m_NextLevel = s_selectedCrc;
                    game->m_bPrepareNextLevel = 1;
                }
                ImGui::EndDisabled();

                if (std::strcmp(currentDesc->m_Category.c_str(), "PvP_Level") == 0)
                    ImGui::EndDisabled();
            }
            else
            {
                ImGui::TextDisabled("Current Level: -");
            }
        }
    }
}
