#pragma once

#include <imgui.h>
#include "game/character.h"
#include "imgui/ui_pages/diagnostics/debug_table.h"

namespace ssa::UIPages
{
    void RenderCharacterTable()
    {
        auto* list = Game::Character::instanceCharactersList();

        ImGui::TextDisabled("Instance: %s", Hex(reinterpret_cast<uintptr_t>(list)).c_str());
        ImGui::Spacing();

        if (!list || list->empty())
        {
            ImGui::TextDisabled("Character List not initialized or empty.");
            return;
        }

        DebugTable table{
            "Address", "Team", "nameID", "Type", "Base HP", "HP Mult", "Atk Mult", "Curr HP", "Last damage",
            "damageRecMask", "dRM Prev", "Pad", "Char Bits", "Char Bits - Binary", "Physics - Send", "Physics - Receive"
        };

        for (const auto& ref : *list)
        {
            auto* ch = ref.mPtr;
            DebugTable::Row& row = table.AddRow();

            row.Dim(Hex(reinterpret_cast<uintptr_t>(ch)));

            switch (ch->m_team)
            {
            case Game::CharacterTeam::Skylander: row.Add("Skylander", TableColor::Green);
                break;
            case Game::CharacterTeam::Enemy: row.Add("Enemy", TableColor::Red);
                break;
            case Game::CharacterTeam::Neutral: row.Add("Neutral", TableColor::Yellow);
                break;
            default: row.Dim(Hex(static_cast<uint32_t>(static_cast<int>(ch->m_team))));
                break;
            }

            row.Dim(Hex(ch->nameID)); // CRC identifying which character type (Zap, Spyro, troll, etc.)

            if (ch->isPlayer()) row.Add(Fmt("Player %d", ch->contID + 1), TableColor::Green);
            else if (ch->isEnemy()) row.Add(Fmt("Enemy (0x%X)", ch->nameID), TableColor::Red);
            else row.Add("Neutral");

            row.Add(Fmt("%.1f", ch->baseHP))
               .Add(Fmt("%.2f", ch->hpMultiplier))
               .Add(Fmt("%.2f", ch->attackMultiplier))
               .Add(Fmt("%.1f", ch->m_fCurrHealth))
               .Add(Fmt("%.1f", ch->m_fLastRemoveHealth))
               .Add(Hex(ch->damageRecMask))
               .Add(Hex(ch->damageRecMaskPrev));

            // which kind of pad controls it
            if (ch->isPlayer()) row.Add("P");
            else if (ch->isLocalAI()) row.Add("AI");
            else if (ch->isRemotePlayer()) row.Add("R");
            else row.Add("-");

            row.Add(Hex(ch->charExtraBits));

            char bits[33];
            for (int b = 0; b < 32; b++)
                bits[31 - b] = (ch->charExtraBits & (1u << b)) ? '1' : '0';
            bits[32] = '\0';
            row.Dim(bits);

            row.Add(Fmt("0x%04X", ch->m_pMotionControl->physicsBody->maskSend))
               .Add(Fmt("0x%04X", ch->m_pMotionControl->physicsBody->maskReceive));
        }

        table.Show("##char_list", ImGuiTableFlags_ScrollY);
    }
} // namespace ssa::UIPages
