#pragma once

#include <imgui.h>
#include "game/character.h"
#include "window/texture_mods.h"

namespace ssa::UIPages
{
    void RenderCharacterTable()
    {
        auto* list = Game::Character::instanceCharactersList();

        ImGui::TextDisabled("Instance: 0x%08X", static_cast<uint32_t>(reinterpret_cast<uintptr_t>(list)));
        ImGui::Spacing();

        if (!list || list->empty())
        {
            ImGui::TextDisabled("Character List not initialized or empty.");
        }
        else if (ImGui::BeginTable("##char_list", 16,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY))
        {
            ImGui::TableSetupColumn("Address");
            ImGui::TableSetupColumn("Team");
            ImGui::TableSetupColumn("nameID");
            ImGui::TableSetupColumn("Type");
            ImGui::TableSetupColumn("Base HP");
            ImGui::TableSetupColumn("HP Mult");
            ImGui::TableSetupColumn("Atk Mult");
            ImGui::TableSetupColumn("Curr HP");
            ImGui::TableSetupColumn("Last damage");
            ImGui::TableSetupColumn("damageRecMask");
            ImGui::TableSetupColumn("damageRecMaskPrev");
            ImGui::TableSetupColumn("invulnFlags");
            ImGui::TableSetupColumn("vulnFlags");
            ImGui::TableSetupColumn("Pad");
            ImGui::TableSetupColumn("Char Bits");
            ImGui::TableSetupColumn("Char Bits - Binary");
            ImGui::TableHeadersRow();

            for (const auto& ref : *list)
            {
                auto* ch = ref.mPtr;

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

                // type
                ImGui::TableSetColumnIndex(3);
                if (ch->isPlayer())
                    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.6f, 1.0f), "Player %d", ch->contID + 1);
                else if (ch->isEnemy())
                    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Enemy (0x%X)", ch->nameID);
                else
                    ImGui::Text("Neutral");

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


                ImGui::TableSetColumnIndex(8);
                ImGui::Text("%.1f", ch->m_fLastRemoveHealth);

                // damageRecMask
                ImGui::TableSetColumnIndex(9);
                ImGui::Text("0x%08X", ch->damageRecMask);

                // damageRecMaskPrev
                ImGui::TableSetColumnIndex(10);
                ImGui::Text("0x%08X", ch->damageRecMaskPrev);

                // invulnFlags
                ImGui::TableSetColumnIndex(11);
                ImGui::Text("0x%08X", ch->invulnFlags);

                // vulnFlags
                ImGui::TableSetColumnIndex(12);
                ImGui::Text("0x%08X", ch->vulnFlags);

                // Pad
                ImGui::TableSetColumnIndex(13);
                if (ch->isPlayer())
                    ImGui::Text("P");
                else if (ch->isLocalAI())
                    ImGui::Text("AI");
                else if (ch->isRemotePlayer())
                    ImGui::Text("R");
                else
                    ImGui::Text("-");


                // raw hex
                ImGui::TableSetColumnIndex(14);
                ImGui::Text("0x%08X", ch->charExtraBits);

                // binary
                ImGui::TableSetColumnIndex(15);
                {
                    char bits[33];
                    for (int b = 0; b < 32; b++)
                        bits[31 - b] = (ch->charExtraBits & (1u << b)) ? '1' : '0';
                    bits[32] = '\0';
                    ImGui::TextDisabled("%s", bits);
                }
            }

            ImGui::EndTable();
        }
    }
} // namespace ssa::UIPages
