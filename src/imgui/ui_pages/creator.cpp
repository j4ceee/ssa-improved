#include <imgui.h>
#include "imgui/ui.h"
#include "config.h"
#include "skylanders/SkylanderManager.h"
#include "imgui/fonts/IconsSkylanders.h"
#include "imgui/fonts/IconsMaterialDesign.h"

namespace ssa::UIPages
{
    void RenderCreator()
    {
        // static state to persist selections between frames
        static int selectedElement = 0;
        static int selectedChar = 0;
        static int selectedVariant = 0;
        static char nicknameBuf[64] = "";
        static std::string statusMsg;
        static bool statusIsError = false;

        // ssa::Skylanders::Element equivalent (sidekicks & items are handled differently)
        const char* elements[] = {
            ICON_SKY_AIR " Air",
            ICON_SKY_EARTH " Earth",
            ICON_SKY_FIRE " Fire",
            ICON_SKY_LIFE " Life",
            ICON_SKY_MAGIC " Magic",
            ICON_SKY_TECH " Tech",
            ICON_SKY_UNDEAD " Undead",
            ICON_SKY_WATER " Water"
        };

        ImGui::BeginDisabled();
        ImGui::TextWrapped("Select a base figure and assign a nickname to generate a save file.");
        ImGui::EndDisabled();
        ImGui::Spacing();

        // --- 1. Element Selection ---
        if (ImGui::Combo("Element", &selectedElement, elements, IM_ARRAYSIZE(elements)))
        {
            // reset downstream selections when the parent changes
            selectedChar = 0;
            selectedVariant = 0;
            statusMsg.clear();
        }

        // --- 2. Filter Characters by Element ---
        std::vector<const Skylanders::SkylanderCategory*> availableChars;
        for (const auto& cat : Skylanders::g_skylanderDb)
        {
            if (static_cast<int>(cat.element) == selectedElement)
            {
                availableChars.push_back(&cat);
            }
        }

        // --- 3. Character Selection ---
        if (!availableChars.empty())
        {
            const char* charPreview = availableChars[selectedChar]->displayName;
            if (ImGui::BeginCombo("Character", charPreview))
            {
                for (int i = 0; i < availableChars.size(); ++i)
                {
                    bool isSelected = (selectedChar == i);
                    if (ImGui::Selectable(availableChars[i]->displayName, isSelected))
                    {
                        selectedChar = i;
                        selectedVariant = 0; // reset variant when character changes
                        statusMsg.clear();
                    }
                    if (isSelected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            // --- 4. Variant Selection ---
            const auto* currentChar = availableChars[selectedChar];
            if (currentChar->numFigures > 0)
            {
                std::string variantPreview = Skylanders::SkylanderManager::GetVariantLabel(currentChar->figures[selectedVariant]);
                if (ImGui::BeginCombo("Variant", variantPreview.c_str()))
                {
                    for (int i = 0; i < currentChar->numFigures; ++i)
                    {
                        bool isSelected = (selectedVariant == i);
                        std::string label = Skylanders::SkylanderManager::GetVariantLabel(currentChar->figures[i]);

                        if (ImGui::Selectable(label.c_str(), isSelected))
                        {
                            selectedVariant = i;
                            statusMsg.clear();
                        }
                        if (isSelected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // --- 5. Nickname & Creation ---
            ImGui::InputTextWithHint("Nickname", "e.g. My Spyro", nicknameBuf, IM_ARRAYSIZE(nicknameBuf));
            ImGui::SameLine();
            UI::HelpMarker("This sets the unique file name (not the nickname of the character in-game).\nAllowed: letters, numbers, hyphens and underscores.");

            ImGui::Spacing();

            bool canCreate = (strlen(nicknameBuf) > 0);
            ImGui::BeginDisabled(!canCreate);
            if (ImGui::Button(ICON_MD_ADD_CIRCLE " Create & Save"))
            {
                uint16_t figId = currentChar->figures[selectedVariant].figureId;
                uint16_t varId = currentChar->figures[selectedVariant].variantId;

                if (Skylanders::SkylanderManager::Get().CreateNewFigure(figId, varId, nicknameBuf))
                {
                    statusMsg = "Success! '" + std::string(nicknameBuf) + "' added to library.";
                    statusIsError = false;
                    nicknameBuf[0] = '\0'; // clear buffer for the next creation
                }
                else
                {
                    statusMsg = "Error: File already exists or name is invalid.";
                    statusIsError = true;
                }
            }
            ImGui::EndDisabled();

            // --- 6. Feedback Text ---
            if (!statusMsg.empty())
            {
                ImGui::SameLine();
                if (statusIsError)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.98f, 0.26f, 0.26f, 1.0f)); // red
                    ImGui::TextWrapped("%s", statusMsg.c_str());
                    ImGui::PopStyleColor();
                }
                else
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 1.0f, 0.4f, 1.0f)); // green
                    ImGui::TextWrapped("%s", statusMsg.c_str());
                    ImGui::PopStyleColor();
                }
            }
        }
    }
} // namespace ssa::UIPages
