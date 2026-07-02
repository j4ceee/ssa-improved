#pragma once

#include <imgui.h>
#include "game/character.h"
#include "game/targeting.h"
#include "window/texture_mods.h"

namespace ssa::UIPages
{
    void RenderTargetTable()
    {
        const auto targeting = Game::TargetingList::instance();

        ImGui::TextDisabled("Instance: 0x%08X", static_cast<uint32_t>(reinterpret_cast<uintptr_t>(targeting)));
        ImGui::Spacing();

        if (!targeting || targeting->empty())
        {
            ImGui::TextDisabled("Target List not initialized or empty.");
        }
        else {
            if (ImGui::BeginTable("##target_list", 5,
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY))
            {
                ImGui::TableSetupColumn("Node Addr");
                ImGui::TableSetupColumn("Object Ptr (mPtr)");
                ImGui::TableSetupColumn("Owner Match");
                ImGui::TableSetupColumn("Mask (Hex)");
                ImGui::TableSetupColumn("Radius");
                ImGui::TableHeadersRow();

                for (const auto& t : *targeting)
                {
                    ImGui::TableNextRow();

                    // node Addr
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextDisabled("0x%08X", static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&t)));

                    // mPtr (lux::Object*)
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextDisabled("0x%08X", static_cast<uint32_t>(reinterpret_cast<uintptr_t>(t.mPtr)));

                    // match to Character
                    ImGui::TableSetColumnIndex(2);
                    bool foundMatch = false;
                    for (const auto& ref : *Game::Character::instanceCharactersList())
                    {
                        if (!ref.mPtr || ref.mPtr->m_pObject != t.mPtr) continue;

                        auto* ch = ref.mPtr;
                        if (ch->isPlayer())
                            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.6f, 1.0f), "Player %d", ch->contID + 1);
                        else if (ch->isEnemy())
                            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Enemy (0x%X)", ch->nameID);
                        else
                            ImGui::Text("Neutral");
                        foundMatch = true;
                        break;
                    }
                    if (!foundMatch) ImGui::TextDisabled("Unknown / None");

                    // mask
                    ImGui::TableSetColumnIndex(3);
                    if (const uint32_t topNibble = t.mask & 0xF0000000; topNibble == 0xC0000000)
                        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.6f, 1.0f), "0x%08X (Sky)", t.mask);
                    else if (topNibble == 0xA0000000)
                        ImGui::TextColored(ImVec4(0.8f, 0.4f, 1.0f, 1.0f), "0x%08X (PvP)", t.mask);
                    else if (topNibble == 0x20000000)
                        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "0x%08X (Enemy)", t.mask);
                    else
                        ImGui::Text("0x%08X", t.mask);

                    // radius
                    ImGui::TableSetColumnIndex(4);
                    ImGui::Text("%.2f", t.radius);
                }
                ImGui::EndTable();
            }
        }
    }
} // namespace ssa::UIPages
