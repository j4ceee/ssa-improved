#include <imgui.h>
#include "game/difficulty.h"
#include "game/game.h"
#include "game/mp_game.h"
#include "imgui/ui.h"
#include "imgui/fonts/IconsMaterialDesign.h"
#include "window/texture_mods.h"

namespace ssa::UIPages
{
    void RenderLevelList(const Game::Game* game)
    {
        const auto& levels = game->m_GamePackage.m_Levels;
        const auto currentLevelId = game->m_CurrLevel;

        if (ImGui::BeginTable("##level_desc_list", 6,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY))
        {
            ImGui::TableSetupColumn("CRC32");
            ImGui::TableSetupColumn("Next Level");
            ImGui::TableSetupColumn("Level Name");
            ImGui::TableSetupColumn("Level File");
            ImGui::TableSetupColumn("Category");
            ImGui::TableSetupColumn("Death Height");
            ImGui::TableHeadersRow();

            for (const auto& desc : levels)
            {
                ImGui::TableNextRow();

                if (desc.m_Id == currentLevelId)
                {
                    ImGui::TableSetBgColor(
                        ImGuiTableBgTarget_RowBg0,
                        ImGui::GetColorU32(ImVec4(0.20f, 0.55f, 0.20f, 0.35f))
                    );
                }

                ImGui::TableSetColumnIndex(0);
                ImGui::Text("0x%08X", desc.m_Id);

                ImGui::TableSetColumnIndex(1);
                ImGui::Text("0x%08X", desc.m_NextLevel);

                ImGui::TableSetColumnIndex(2);
                if (auto* name = Game::Data::GetLevelDisplayInfo(desc.m_Id))
                    ImGui::Text("%s", name->displayName);
                else
                    ImGui::TextDisabled("-");

                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%s", desc.m_LevelFile.c_str());

                ImGui::TableSetColumnIndex(4);
                ImGui::Text("%s", desc.m_Category.c_str());

                ImGui::TableSetColumnIndex(5);
                ImGui::Text("%.1f", desc.m_fDeathHeight);
            }
            ImGui::EndTable();
        }
    }
    void RenderCharacterList(const Game::Game* game)
    {
        const auto& chars = game->m_GamePackage.m_Characters;

        if (ImGui::BeginTable("##char_desc_list", 6,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY))
        {
            ImGui::TableSetupColumn("CRC32");
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Name String ID");
            ImGui::TableSetupColumn("File");
            ImGui::TableSetupColumn("Attributes");
            ImGui::TableSetupColumn("Scale");
            ImGui::TableHeadersRow();

            for (const auto& desc : chars)
            {
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::Text("0x%08X", desc.m_Id);

                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s", desc.m_Name.c_str());

                ImGui::TableSetColumnIndex(2);
                ImGui::Text("0x%08X", desc.m_NameStrId);

                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%s", desc.m_FilePath.c_str());

                ImGui::TableSetColumnIndex(4);
                ImGui::Text("0x%08X", desc.m_characterAttributes);

                ImGui::TableSetColumnIndex(5);
                ImGui::Text("%.1f", desc.m_fScale);
            }
            ImGui::EndTable();
        }
    }
    void RenderMagicItemList(const Game::Game* game)
    {
        const auto& items = game->m_GamePackage.m_MagicItems;

        if (ImGui::BeginTable("##char_desc_list", 3,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY))
        {
            ImGui::TableSetupColumn("CRC32");
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Type");
            ImGui::TableHeadersRow();

            for (const auto& desc : items)
            {
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::Text("0x%08X", desc.m_Id);

                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s", desc.m_Name.c_str());

                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%d", desc.m_Type);
            }
            ImGui::EndTable();
        }
    }

    void RenderDiagnosticWindow()
    {
        if (ImGui::TreeNode("Game"))
        {
            ImGui::Indent();
            auto* game = Game::Game::instance();
            ImGui::TextDisabled("Instance: 0x%08X", static_cast<uint32_t>(reinterpret_cast<uintptr_t>(game)));
            ImGui::TextDisabled("Current Level: 0x%08X", game->m_CurrLevel);
            ImGui::TextDisabled("Paused: %d", game->m_bGameFrozen);
            ImGui::TextDisabled("Death Height: %.1f", game->m_fDeathHeight);
            ImGui::TextDisabled("Player Idle Time: %.1f", game->m_fPlayerIdleTime);
            ImGui::TextDisabled("Preparing next level: %d", game->m_bPrepareNextLevel);
            ImGui::TextDisabled("Game speed: %.1f", game->gameSpeeds.currRatio);

            if (ImGui::TreeNode("Level List"))
            {
                ImGui::Indent();
                RenderLevelList(game);
                ImGui::Unindent();
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Skylander List"))
            {
                ImGui::Indent();
                RenderCharacterList(game);
                ImGui::Unindent();
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Magic Item List"))
            {
                ImGui::Indent();
                RenderMagicItemList(game);
                ImGui::Unindent();
                ImGui::TreePop();
            }

            ImGui::Unindent();
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("MP Game"))
        {
            ImGui::Indent();
            auto* mpGame = Game::MPGame::instance();
            ImGui::TextDisabled("Instance: 0x%08X", static_cast<uint32_t>(reinterpret_cast<uintptr_t>(mpGame)));
            ImGui::TextDisabled("State: %d", mpGame->m_bIsPvPMatch);
            ImGui::TextDisabled("Rules: 0x%08X", static_cast<uint32_t>(reinterpret_cast<uintptr_t>(mpGame->m_pRules)));

            if (ImGui::Button("Toggle PvP Match"))
            {
                mpGame->SetInPvPMatch(!mpGame->isInPvPMatch());
            }
            ImGui::SameLine();
            UI::HelpMarker("Put one player in enemy team for melee attacks to register while PvP is active");

            ImGui::Unindent();
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Damage"))
        {
            ImGui::Indent();
            auto* scs = Game::SpyroCharacterSettings::instance();
            auto& enemy = scs->m_enemySettings;
            const bool levelLoaded = enemy.m_pLevelAttribute != nullptr;

            if (levelLoaded)
                ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.6f, 1.0f), ICON_MD_CHECK_CIRCLE " Level loaded");
            else
                ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.1f, 1.0f), ICON_MD_WARNING " Not in a level");

            ImGui::TextDisabled("Instance: 0x%08X", static_cast<uint32_t>(reinterpret_cast<uintptr_t>(scs)));
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

            ImGui::Unindent();
            ImGui::TreePop();
        }

        ImGui::Spacing();
        ImGui::Spacing();

        if (ImGui::Button("Open Character Table"))
            UI::Get()->SetCharacterTableVisible(true);

        ImGui::Spacing();

        if (ImGui::Button("Open Target Table"))
            UI::Get()->SetTargetTableVisible(true);
    }
} // namespace ssa::UIPages
