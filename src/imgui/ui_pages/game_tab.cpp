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
            UI::HelpMarker("Scales enemy HP. Enemies will be tougher to defeat when this value is increased. Default = 1.0");

            ImGui::SliderFloat("Enemy damage multiplier", &g_config.dmgMult, 0.1f, 10.0f, "%.2fx");
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                SetEnemyDmgMultiplier(g_config.dmgMult);
            }
            ImGui::SameLine();
            UI::HelpMarker("Scales enemy damage. Enemies will deal more damage when this value is increased. Default = 1.0");

            ImGui::Spacing();
            ImGui::Spacing();

            if (ImGui::Checkbox("Enemy hit reaction", &g_config.enemyHitReaction))
            {
                SetEnemyHitReaction(g_config.enemyHitReaction);
            }
            ImGui::SameLine();
            UI::HelpMarker("Enable or disable enemy hit reaction. Disable to make enemies attack faster & not be stunned by player attacks.");

            ImGui::Spacing();
            ImGui::Spacing();

            ImGui::SliderFloat("Heroic Challenge HP ceiling", &g_config.heroicHpCeiling, 0.1f, 10.0f, "%.2fx");
            if (ImGui::IsItemDeactivatedAfterEdit())
                SetEnemyHpHeroicCeiling(g_config.heroicHpCeiling);
            ImGui::SameLine();
            UI::HelpMarker("Maximum HP enemies can have in Heroic Challenges. If you find that enemies in challenges are too weak, try increasing this. Default = 3.0");

            ImGui::SliderFloat("Heroic Challenge damage ceiling", &g_config.heroicDmgCeiling, 0.1f, 10.0f, "%.2fx");
            if (ImGui::IsItemDeactivatedAfterEdit())
                SetEnemyDmgHeroicCeiling(g_config.heroicDmgCeiling);
            ImGui::SameLine();
            UI::HelpMarker("Maximum damage enemies can deal in Heroic Challenges. If you find that enemies in challenges are too weak, try increasing this. Default = 0.1");
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
            UI::HelpMarker("Multiplies the amount of XP gained from defeating enemies. Default = 1.0");
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

                    bool isPlayer1 = ch->isPlayer1();

                    ImGui::PushID(static_cast<int>(reinterpret_cast<uintptr_t>(ch)));
                    ImGui::SeparatorText(isPlayer1 ? "Player 1" : "Player 2");

                    // Health slider - live write to m_fCurrHealth
                    ImGui::SliderFloat("HP", &ch->m_fCurrHealth, 0.0f, ch->maxHP(), "%.0f");
                    ImGui::SameLine();
                    UI::HelpMarker("Current health. Drag to set HP directly.");

                    ImGui::Spacing();

                    // God mode
                    ImGui::Checkbox("God mode", isPlayer1 ? &g_config.p1GodMode : &g_config.p2GodMode);
                    ImGui::SameLine();
                    UI::HelpMarker("Makes the Skylander immune to all damage.");

                    // No knockback
                    ImGui::Checkbox("No knockback", isPlayer1 ? &g_config.p1NoKnockback : &g_config.p2NoKnockback);
                    ImGui::SameLine();
                    UI::HelpMarker("Prevents the Skylander from being knocked back by attacks.");

                    // No hit reaction
                    ImGui::Checkbox("No hit reaction", isPlayer1 ? &g_config.p1NoHitReaction : &g_config.p2NoHitReaction);
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
                    UI::HelpMarker("Change the faction of the Skylander. Members of the same faction cannot attack the Skylander.");

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
