#include "imgui/ui.h"
#include "imgui/fonts/IconsMaterialDesign.h"
#include <imgui.h>
#include "config.h"
#include "game/difficulty.h"
#include "game/character.h"
#include "imgui/fonts/IconsSkylanders.h"

namespace ssa::UIPages
{
    void RenderGameTab()
    {
        auto* scs = Game::SpyroCharacterSettings::instance();
        auto& enemy = scs->m_enemySettings;
        bool levelLoaded = enemy.m_pLevelAttribute != nullptr;

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

            // Diagnostics
            // -------------------------------------------------------------------------------------------------
            ImGui::Spacing();
            if (ImGui::TreeNode(ICON_MD_BUG_REPORT " Diagnostics"))
            {
                if (levelLoaded)
                    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.6f, 1.0f), ICON_MD_CHECK_CIRCLE " Level loaded");
                else
                    ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.1f, 1.0f), ICON_MD_WARNING " Not in a level");

                ImGui::TextDisabled("Singleton 0x%08X", static_cast<uint32_t>(reinterpret_cast<uintptr_t>(scs)));
                ImGui::TextDisabled("LevelAttr 0x%08X", static_cast<uint32_t>(reinterpret_cast<uintptr_t>(enemy.m_pLevelAttribute)));
                ImGui::TextDisabled("Snapshot 0x%08X", static_cast<uint32_t>(reinterpret_cast<uintptr_t>(Difficulty::s_snapshotAttr)));

                ImGui::Spacing();

                static constexpr const char* kClassNames[8] = {
                    "0 - Swarmer", "1 - Spawner", "2 - Unknown", "3 - Unknown", "4 - Unknown", "5 - Unknown", "6 - Unknown", "7 - Unknown",
                };

                if (ImGui::BeginTable("##enemy_stats", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit))
                {
                    ImGui::TableSetupColumn("Class");
                    ImGui::TableSetupColumn("DMG (raw)");
                    ImGui::TableSetupColumn("DMG (current)");
                    ImGui::TableHeadersRow();

                    for (int i = 0; i < 8; i++)
                    {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("%s", kClassNames[i]);

                        if (levelLoaded)
                        {
                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%.2f", Difficulty::s_baseDmgSnapshot[i]);
                            ImGui::TableSetColumnIndex(2);
                            ImGui::Text("%.2f", enemy.m_fBaseDamage[i]);
                        }
                        else
                        {
                            for (int c = 1; c < 3; c++)
                            {
                                ImGui::TableSetColumnIndex(c);
                                ImGui::TextDisabled("--");
                            }
                        }
                    }
                    ImGui::EndTable();
                }

                {
                    auto* list = Game::CharacterList::instanceAll();

                    // limit to 64
                    static constexpr int kMaxChars = 64;
                    Game::Character* chars[kMaxChars];
                    int count = 0;

                    for (auto* node = list->begin(); node != list->end() && count < kMaxChars; node = node->next)
                    {
                        if (node->ptr)
                            chars[count++] = node->ptr;
                    }

                    ImGui::TextDisabled("Characters in list: %d", count);
                    ImGui::TextDisabled("Sentinel 0x%08X", static_cast<uint32_t>(reinterpret_cast<uintptr_t>(list)));
                    ImGui::Spacing();

                    if (count == 0)
                    {
                        ImGui::TextDisabled("No characters found.");
                    }
                    else if (ImGui::BeginTable("##char_list", 8,
                                               ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                               ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY,
                                               ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * (count + 2))))
                    {
                        ImGui::TableSetupColumn("Address");
                        ImGui::TableSetupColumn("Team");
                        ImGui::TableSetupColumn("nameID"); // +0x15C: crc32 identifying character type
                        ImGui::TableSetupColumn("Attrs"); // +0x11C: non-null = regular enemy; null = boss Character
                        ImGui::TableSetupColumn("Base HP");
                        ImGui::TableSetupColumn("HP Mult");
                        ImGui::TableSetupColumn("Atk Mult"); // +0x1E0: PDB-sourced, plausible values seen in-game
                        ImGui::TableSetupColumn("Curr HP");
                        ImGui::TableHeadersRow();

                        for (int i = 0; i < count; i++)
                        {
                            auto* ch = chars[i];

                            ImGui::TableNextRow();

                            // Address
                            ImGui::TableSetColumnIndex(0);
                            ImGui::TextDisabled("0x%08X", static_cast<uint32_t>(reinterpret_cast<uintptr_t>(ch)));

                            // Team
                            ImGui::TableSetColumnIndex(1);
                            switch (ch->m_team)
                            {
                            case Game::CharacterTeam::Skylander:
                                ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.6f, 1.0f), "Skylander");
                                break;
                            case Game::CharacterTeam::Enemy:
                                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Enemy");
                                break;
                            case Game::CharacterTeam::Neutral:
                                ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.4f, 1.0f), "Neutral");
                                break;
                            default:
                                ImGui::TextDisabled("0x%08X", static_cast<uint32_t>(static_cast<int>(ch->m_team)));
                                break;
                            }

                            // nameID - CRC identifying which character type (Zap, Spyro, troll, etc.)
                            ImGui::TableSetColumnIndex(2);
                            ImGui::TextDisabled("0x%08X", ch->nameID);

                            // m_pAttributes - null = boss Character bypassing EnemySettings
                            ImGui::TableSetColumnIndex(3);
                            if (ch->m_pAttributes != nullptr)
                                ImGui::TextDisabled("yes");
                            else
                                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "null");

                            // Base HP
                            ImGui::TableSetColumnIndex(4);
                            ImGui::Text("%.1f", ch->baseHP);

                            // HP multiplier
                            ImGui::TableSetColumnIndex(5);
                            ImGui::Text("%.2f", ch->hpMultiplier);

                            // Attack multiplier
                            ImGui::TableSetColumnIndex(6);
                            ImGui::Text("%.2f", ch->attackMultiplier);

                            // Current HP
                            ImGui::TableSetColumnIndex(7);
                            ImGui::Text("%.1f", ch->m_fCurrHealth);
                        }

                        ImGui::EndTable();
                    }
                }

                ImGui::TreePop();
            }
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
            auto* list = Game::CharacterList::instanceSkylanders();

            if (list->begin() == list->end())
            {
                ImGui::TextDisabled("No Skylanders on portal.");
            }
            else
            {
                for (auto* node = list->begin(); node != list->end(); node = node->next)
                {
                    auto* ch = node->ptr;
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

    }
}
