#include <imgui.h>
#include "game/character.h"
#include "game/targeting.h"
#include "imgui/ui_pages/diagnostics/debug_table.h"

namespace ssa::UIPages
{
    void RenderTargetTable()
    {
        const auto targeting = Game::TargetingList::instance();

        ImGui::TextDisabled("Instance: %s", Hex(reinterpret_cast<uintptr_t>(targeting)).c_str());
        ImGui::Spacing();

        if (!targeting || targeting->empty())
        {
            ImGui::TextDisabled("Target List not initialized or empty.");
            return;
        }

        const auto* characters = Game::Character::instanceCharactersList();

        DebugTable table{ "Node Addr", "Object Ptr (mPtr)", "Owner Match", "Mask (Hex)", "Radius" };
        for (const auto& t : *targeting)
        {
            DebugTable::Row& row = table.AddRow();
            row.Dim(Hex(reinterpret_cast<uintptr_t>(&t)))
               .Dim(Hex(reinterpret_cast<uintptr_t>(t.mPtr)));

            // match to Character
            bool foundMatch = false;
            if (characters)
            {
                for (const auto& ref : *characters)
                {
                    auto* ch = ref.mPtr;
                    if (!ch || ch->m_pObject != t.mPtr) continue;

                    if (ch->isPlayer())     row.Add(Fmt("Player %d", ch->contID + 1), TableColor::Green);
                    else if (ch->isEnemy()) row.Add(Fmt("Enemy (0x%X)", ch->nameID), TableColor::Red);
                    else                    row.Add("Neutral");
                    foundMatch = true;
                    break;
                }
            }
            if (!foundMatch) row.Dim("Unknown / None");

            if (const uint32_t topNibble = t.mask & 0xF0000000; topNibble == 0xC0000000)
                row.Add(Fmt("0x%08X (Sky)", t.mask), TableColor::Green);
            else if (topNibble == 0xA0000000)
                row.Add(Fmt("0x%08X (PvP)", t.mask), TableColor::Purple);
            else if (topNibble == 0x20000000)
                row.Add(Fmt("0x%08X (Enemy)", t.mask), TableColor::Red);
            else
                row.Add(Hex(t.mask));

            row.Add(Fmt("%.2f", t.radius));
        }

        table.Show("##target_list", ImGuiTableFlags_ScrollY);
    }
} // namespace ssa::UIPages
