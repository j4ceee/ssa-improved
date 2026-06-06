#include "imgui/ui.h"
#include "imgui/fonts/IconsMaterialDesign.h"
#include <imgui.h>
#include "config.h"
#include "skylanders/SkylanderManager.h"
#include "portal/portal_device.h"
#include "window/d3d9_hooks.h"

#include "imgui/fonts/IconsSkylanders.h"
#include "portal/backend/EmulatedBackend.h"

#define STB_IMAGE_IMPLEMENTATION
#include <d3d9.h>

#include "imgui/images/stb_image.h"
#include "imgui/images/portal_frame_data.h"
#include "imgui/images/portal_content_data.h"

namespace ssa::UIPages
{
    // -------------------------------------------------------------------------
    // Helpers
    // -------------------------------------------------------------------------

    static IDirect3DTexture9* LoadEmbeddedTexture(IDirect3DDevice9* dev, const unsigned char* buf, unsigned int bufLen,
                                                  ImVec2* outSize = nullptr)
    {
        int w, h, ch;
        unsigned char* px = stbi_load_from_memory(buf, (int)bufLen, &w, &h, &ch, 4);
        if (!px) return nullptr;

        IDirect3DTexture9* tex = nullptr;
        if (FAILED(dev->CreateTexture(w, h, 1, 0,
            D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &tex, nullptr)))
        {
            stbi_image_free(px);
            return nullptr;
        }

        D3DLOCKED_RECT lr{};
        tex->LockRect(0, &lr, nullptr, 0);
        for (int y = 0; y < h; ++y)
        {
            auto* dst = reinterpret_cast<uint32_t*>(
                static_cast<uint8_t*>(lr.pBits) + y * lr.Pitch);
            const uint8_t* src = px + y * w * 4;
            for (int x = 0; x < w; ++x)
            {
                uint8_t r = src[x * 4], g = src[x * 4 + 1], b = src[x * 4 + 2], a = src[x * 4 + 3];
                dst[x] = (a << 24) | (r << 16) | (g << 8) | b; // → A8R8G8B8
            }
        }
        tex->UnlockRect(0);
        stbi_image_free(px);

        if (outSize) *outSize = {(float)w, (float)h};
        return tex;
    }

    ImVec4 GetElementColor(Skylanders::Element element)
    {
        switch (element)
        {
        case Skylanders::Element::AIR: return {0.65f, 0.65f, 0.85f, 0.8f};
        case Skylanders::Element::EARTH: return {0.6f, 0.4f, 0.2f, 0.8f};
        case Skylanders::Element::FIRE: return {0.9f, 0.2f, 0.1f, 0.8f};
        case Skylanders::Element::LIFE: return {0.2f, 0.8f, 0.2f, 0.8f};
        case Skylanders::Element::MAGIC: return {0.6f, 0.1f, 0.6f, 0.8f};
        case Skylanders::Element::TECH: return {0.9f, 0.6f, 0.1f, 0.8f};
        case Skylanders::Element::UNDEAD: return {0.4f, 0.4f, 0.4f, 0.8f};
        case Skylanders::Element::WATER: return {0.1f, 0.4f, 0.9f, 0.8f};
        default: return {0.5f, 0.5f, 0.5f, 0.8f};
        }
    }

    // -------------------------------------------------------------------------
    // Portal texture state
    // -------------------------------------------------------------------------
    static IDirect3DTexture9* s_portalFrameTex = nullptr;
    static IDirect3DTexture9* s_portalGlowTex = nullptr;
    static ImVec2 s_portalTexSize = {};
    static bool s_texInitDone = false;

    static void EnsurePortalTextures()
    {
        if (s_texInitDone) return;

        IDirect3DDevice9* dev = D3D9Hooks::g_d3dDevice;
        if (!dev) return; // retry next frame

        s_texInitDone = true; // only commit once we know the device is ready

        s_portalFrameTex = LoadEmbeddedTexture(dev, portal_bg, portal_bg_len, &s_portalTexSize);
        s_portalGlowTex = LoadEmbeddedTexture(dev, portal_fg, portal_fg_len, nullptr);
    }

    void ReleasePortalTextures()
    {
        if (s_portalFrameTex)
        {
            s_portalFrameTex->Release();
            s_portalFrameTex = nullptr;
        }
        if (s_portalGlowTex)
        {
            s_portalGlowTex->Release();
            s_portalGlowTex = nullptr;
        }
        s_texInitDone = false;
    }

    // -------------------------------------------------------------------------
    // Portal Visual
    //
    // Tints the glowing oval with the colour the game has set on the portal, falling back to cyan when no colour has been received yet.
    // -------------------------------------------------------------------------
    static void DrawPortalVisual()
    {
        EnsurePortalTextures();

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 pos = ImGui::GetCursorScreenPos();
        float w = ImGui::GetContentRegionAvail().x;

        // base colour: portal LED colour if available, otherwise cyan
        float br = 0.0f, bg = 0.8f, bb = 1.0f;
        bool useGameColor = false; // if false it uses the base colour above

        auto col = Portal::GetPortalColor();
        if (col.r | col.g | col.b) // non-black = game has set a colour
        {
            br = col.r / 255.0f;
            bg = col.g / 255.0f;
            bb = col.b / 255.0f;

            // pass 1: lift raw LED values to a minimum channel brightness
            constexpr float kMinChannel = 0.75f;
            float maxC = std::max({br, bg, bb});
            if (maxC > 0.001f && maxC < kMinChannel)
            {
                float s = kMinChannel / maxC;
                br = std::min(br * s, 1.0f);
                bg = std::min(bg * s, 1.0f);
                bb = std::min(bb * s, 1.0f);
            }

            // pass 2: perceptual luminance equalisation
            // red / blue look much darker than cyan / yellow
            // -> R=0.21, G=0.72, B=0.07 (Rec.709).
            // https://en.wikipedia.org/wiki/Rec._709#Luma_coefficients
            constexpr float kLr = 0.2125f, kLg = 0.7154f, kLb = 0.0721f;
            constexpr float kTargetLum = 0.45f;
            constexpr float kStrength   = 0.70f; // 0 = no correction, 1 = full equalisation
            float lum = kLr * br + kLg * bg + kLb * bb;
            if (lum > 0.001f && lum < kTargetLum)
            {
                float boost = (kTargetLum - lum) * kStrength;
                br = std::min(br + boost, 1.0f);
                bg = std::min(bg + boost, 1.0f);
                bb = std::min(bb + boost, 1.0f);
            }

            useGameColor = true;
        }

        ImVec4 tint;
        if (useGameColor)
        {
            // no pulse on game color
            tint = ImVec4(br, bg, bb, 1.0f);
        }
        else
        {
            float t = static_cast<float>(ImGui::GetTime());
            float pulse = (sinf(t * 2.0f) + 1.0f) * 0.5f;
            // pulse only the cyan fallback
            float bright = 0.6f + pulse * 0.2f;
            tint = ImVec4(br * bright, bg * bright, bb * bright, 1.0f);
        }

        if (s_portalFrameTex && s_portalGlowTex)
        {
            float aspect = s_portalTexSize.y / s_portalTexSize.x;
            float h = w * aspect;
            ImVec2 pMax = ImVec2(pos.x + w, pos.y + h);

            ImU32 glowTint = ImGui::ColorConvertFloat4ToU32(tint);

            // frame first (original colours), glow layer on top
            dl->AddImage(s_portalFrameTex, pos, pMax, {0, 0}, {1, 1}, IM_COL32_WHITE);
            dl->AddImage(s_portalGlowTex, pos, pMax, {0, 0}, {1, 1}, glowTint);

            ImGui::Dummy(ImVec2(w, h));
        }
    }

    // -------------------------------------------------------------------------
    // Main render function
    // -------------------------------------------------------------------------
    void RenderPortalTab()
    {
        bool emuEnabled = g_config.emulatedPortal;
        if (ImGui::Checkbox("Enable Emulated Portal", &emuEnabled))
        {
            SetEmulatedPortal(emuEnabled);
        }
        ImGui::SameLine();
        UI::HelpMarker(
            "When enabled, the mod will ignore the physical portal and emulate all portal interactions in software.",
            "Restart your game for this setting to take effect");

        if (!Portal::g_backend)
        {
            // no backend!
            ImGui::Spacing();
            UI::WarningText(
                "\n\nNo portal backend available!\n\n"
                "Restart your game and make sure you have a supported portal plugged in at game startup or enable the emulated portal!",
                true);
            return;
        }

        auto* emu = Portal::GetEmulated();

        // --- 1. Portal Visual ---
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Spacing();
        DrawPortalVisual();
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Spacing();

        // --- 2. Active Slots ---
        if (!emu)
        {
            // physical portal (or no backend) -> read-only slot display
            // show slot occupancy from S packets (populated for all backends)
            ImGui::SeparatorText("Active Skylanders");
            ImGui::Spacing();
            ImGui::TextDisabled("Physical portal (read-only)");
            ImGui::Spacing();

            auto slots = Portal::GetSlotInfo();

            // count occupied slots to decide what to render
            int occupiedCount = 0;
            for (int i = 0; i < 8; i++)
                if (slots[i].present) occupiedCount++;

            ImGui::Indent();
            if (occupiedCount == 0)
            {
                ImGui::TextDisabled("No figures on portal");
            }
            else if (ImGui::BeginTable("PhysicalSlots", 2,
                ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingFixedFit))
            {
                ImGui::TableSetupColumn("Slot",   ImGuiTableColumnFlags_WidthFixed,   ImGui::GetFontSize() * 4.0f);
                ImGui::TableSetupColumn("Figure", ImGuiTableColumnFlags_WidthStretch);

                for (int i = 0; i < 8; i++)
                {
                    if (!slots[i].present) continue; // skip empty slots

                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Slot %d:", i + 1);
                    ImGui::TableNextColumn();

                    if (slots[i].figureId != 0)
                    {
                        const auto* info = Skylanders::SkylanderManager::FindFigure(slots[i].figureId, slots[i].variantId);
                        if (info)
                            ImGui::Text("%s", Skylanders::SkylanderManager::GetVariantLabel(*info).c_str());
                        else
                            ImGui::Text("Unknown (ID %u)", slots[i].figureId);
                    }
                    else
                    {
                        // present but block 1 not yet received (game will query it shortly)
                        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Skylander detected...");
                    }
                }
                ImGui::EndTable();
            }
            ImGui::Unindent();
            return;
        }


        // activeChangeSlot: -1 = none, 0-3 = picking a figure for that slot
        static int activeChangeSlot = -1;

        ImGui::SeparatorText("Active Skylanders");
        ImGui::Spacing();

        // first empty slot gets a Set button, occupied slots get Set+Clear.
        // later empty slots show nothing, they unlock as figures are added
        int firstEmptySlot = Portal::Backend::EmulatedBackend::SLOT_COUNT;
        for (int i = 0; i < Portal::Backend::EmulatedBackend::SLOT_COUNT; i++)
        {
            if (!emu->GetSlotDisplay(i).occupied)
            {
                firstEmptySlot = i;
                break;
            }
        }

        ImGui::Indent();
        if (ImGui::BeginTable("SlotsTable", 4, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingFixedFit))
        {
            ImGui::TableSetupColumn("Slot", ImGuiTableColumnFlags_WidthFixed, ImGui::GetFontSize() * 4.0f);
            ImGui::TableSetupColumn("Figure", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Set", ImGuiTableColumnFlags_WidthFixed, ImGui::GetFontSize() * 5.0f);
            ImGui::TableSetupColumn("Clear", ImGuiTableColumnFlags_WidthFixed, ImGui::GetFontSize() * 4.5f);

            for (int i = 0; i < Portal::Backend::EmulatedBackend::SLOT_COUNT; i++)
            {
                auto display = emu->GetSlotDisplay(i);
                bool isPicking = (activeChangeSlot == i);
                bool showSet = display.occupied || (i == firstEmptySlot) || isPicking;
                bool showClear = display.occupied;

                ImGui::TableNextRow();

                // column 0 - slot label
                ImGui::TableNextColumn();
                ImGui::Text("Slot %d:", i + 1);

                // column 1 - figure name (or empty indicator)
                ImGui::TableNextColumn();
                if (display.occupied && !display.filePath.empty())
                {
                    // resolve name from the library so we show the friendly display name
                    std::string label = display.filePath.stem().string();
                    const auto& lib = Skylanders::SkylanderManager::Get().GetLibrary();
                    for (const auto& entry : lib)
                    {
                        if (entry.filePath == display.filePath)
                        {
                            label = entry.displayName;
                            if (!entry.nickName.empty())
                                label += " (\"" + entry.nickName + "\")";
                            break;
                        }
                    }
                    ImGui::TextUnformatted(label.c_str());
                }
                else
                {
                    ImGui::TextDisabled("--- Empty ---");
                }

                // column 2 - set / cancel button
                ImGui::TableNextColumn();
                if (showSet)
                {
                    if (isPicking)
                    {
                        if (ImGui::Button(("Cancel##s" + std::to_string(i)).c_str(), ImVec2(-1, 0)))
                            activeChangeSlot = -1;
                    }
                    else
                    {
                        if (ImGui::Button(("Set##s" + std::to_string(i)).c_str(), ImVec2(-1, 0)))
                            activeChangeSlot = i;
                    }
                }

                // column 3 - clear button (occupied slots only)
                ImGui::TableNextColumn();
                if (showClear)
                {
                    if (ImGui::Button(("Clear##s" + std::to_string(i)).c_str(), ImVec2(-1, 0)))
                    {
                        emu->RemoveSkylander(i);
                        if (activeChangeSlot == i)
                            activeChangeSlot = -1;
                    }
                }
            }
            ImGui::EndTable();
        }
        ImGui::Unindent();

        ImGui::Spacing();
        ImGui::Spacing();

        // --- 3. Library ---
        ImGui::SeparatorText("Library");
        ImGui::Spacing();

        if (ImGui::Button(ICON_MD_ADD " Open Skylander Creator"))
            UI::Get()->SetCreatorVisible(true);

        ImGui::Spacing();

        if (activeChangeSlot == -1)
        {
            ImGui::TextDisabled("(Click 'Set' on a slot above to assign a Skylander)");
            ImGui::Spacing();
        }

        // dim the library when no slot is being changed
        ImGui::BeginDisabled(activeChangeSlot == -1);

        const auto& library = Skylanders::SkylanderManager::Get().GetLibrary();

        const char* elementIcons[] = {
            ICON_SKY_AIR, ICON_SKY_EARTH, ICON_SKY_FIRE, ICON_SKY_LIFE,
            ICON_SKY_MAGIC, ICON_SKY_TECH, ICON_SKY_UNDEAD, ICON_SKY_WATER,
            ICON_SKY_MINIS, ICON_SKY_NEW
        };

        const char* elementNames[] = {
            "Air", "Earth", "Fire", "Life", "Magic", "Tech", "Undead", "Water"
        };
        constexpr int numElements = 8;

        int currentElement = -1;
        bool colorsPushed = false;
        bool elementHeaderOpen = false;

        for (const auto& charCat : Skylanders::g_skylanderDb)
        {
            const int elemIdx = static_cast<int>(charCat.element);
            if (elemIdx >= numElements) continue; // skip SIDEKICK / ITEM

            // collect owned variants, checks all figure IDs in the category, not just [0]
            std::vector<const Skylanders::LoadedSkylander*> owned;
            for (const auto& sky : library)
            {
                for (size_t f = 0; f < charCat.numFigures; ++f)
                {
                    if (sky.figureId == charCat.figures[f].figureId &&
                        sky.variantId == charCat.figures[f].variantId)
                    {
                        owned.push_back(&sky);
                        break;
                    }
                }
            }
            if (owned.empty()) continue;

            // element transition, close previous section, open new one
            if (elemIdx != currentElement)
            {
                if (elementHeaderOpen) ImGui::Unindent();
                if (colorsPushed) ImGui::PopStyleColor(6);

                currentElement = elemIdx;
                ImVec4 hdrColor = GetElementColor(static_cast<Skylanders::Element>(elemIdx));
                ImGui::PushStyleColor(ImGuiCol_Header, hdrColor);
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(hdrColor.x * 1.2f, hdrColor.y * 1.2f, hdrColor.z * 1.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(hdrColor.x * 0.8f, hdrColor.y * 0.8f, hdrColor.z * 0.8f, 1.0f));

                ImGui::PushStyleColor(ImGuiCol_Button, hdrColor);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(hdrColor.x * 1.2f, hdrColor.y * 1.2f, hdrColor.z * 1.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(hdrColor.x * 1.2f, hdrColor.y * 1.2f, hdrColor.z * 1.2f, 1.0f));
                colorsPushed = true;

                std::string headerText = std::string(elementIcons[elemIdx]) + " " + elementNames[elemIdx];
                elementHeaderOpen = ImGui::CollapsingHeader(headerText.c_str());
                if (elementHeaderOpen) ImGui::Indent();
            }

            if (!elementHeaderOpen) continue;

            // character sub-header
            if (ImGui::CollapsingHeader(charCat.displayName))
            {
                ImGui::Indent();

                ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f)); // left-align text in buttons
                for (const auto* variant : owned)
                {
                    std::string label = variant->displayName;
                    if (!variant->nickName.empty())
                        label += " (\"" + variant->nickName + "\")";
                    label += "##" + variant->filename;

                    if (ImGui::ButtonEx(label.c_str(), ImVec2(-1, 0)) && activeChangeSlot >= 0)
                    {
                        emu->LoadSkylander(activeChangeSlot, variant->filePath);
                        activeChangeSlot = -1;
                    }
                }
                ImGui::PopStyleVar();
                ImGui::Unindent();
            }
        }

        // close the last open element section
        if (elementHeaderOpen) ImGui::Unindent();
        if (colorsPushed) ImGui::PopStyleColor(6);

        ImGui::EndDisabled();
    }
} // namespace ssa::UIPages
