#include "../ui.h"
#include "../fonts/IconsMaterialDesign.h"
#include <imgui.h>
#include "config.h"

namespace ssa::UIPages
{
    void RenderSettingsTab()
    {
        // Mod
        // -----------------------------------------------------------------------------------------------------
        if (ImGui::CollapsingHeader(ICON_MD_BUILD " Mod", ImGuiTreeNodeFlags_DefaultOpen))
        {
            float fontScale = g_config.uiFontScale;
            if (ImGui::DragFloat("UI Font Scale", &fontScale, 0.1f, 0.5f, 3.0f, "%.1f"))
            {
                SetFontScale(fontScale);
            }
            ImGui::SameLine();
            UI::HelpMarker("Adjusts the scale of the UI font.");
        }

        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Spacing();

        // Graphics
        // -----------------------------------------------------------------------------------------------------
        if (ImGui::CollapsingHeader(ICON_MD_IMAGE " Graphics", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Spacing();

            // fps cap
            int fpsCap = g_config.fpsCap;
            if (ImGui::InputInt("FPS Cap", &fpsCap, 5, 20))
                SetFpsCap(fpsCap);
            ImGui::SameLine();
            UI::HelpMarker("Limits the maximum frames per second (FPS) the game will render. 0 = unlimited");


            // anisotropic filtering
            const int afLevels[] = {1, 2, 4, 8, 16};
            const char* afLabels[] = {"Off", "2x", "4x", "8x", "16x"};
            int currentAfIndex = 0;
            int currentAfLevel = g_config.anisotropy;
            if (currentAfLevel <= 1) currentAfIndex = 0;
            else if (currentAfLevel <= 2) currentAfIndex = 1;
            else if (currentAfLevel <= 4) currentAfIndex = 2;
            else if (currentAfLevel <= 8) currentAfIndex = 3;
            else currentAfIndex = 4;

            if (ImGui::Combo("Anisotropic Filtering", &currentAfIndex, afLabels, 5))
            {
                SetAnisotropy(afLevels[currentAfIndex]);
            }
            ImGui::SameLine();
            UI::HelpMarker("Improves the clarity of textures viewed at oblique angles. Higher levels provide better quality but may reduce performance on older GPUs.");


            // texture sharpness
            int sharpness = g_config.textureSharpness;
            if (ImGui::SliderInt("Texture Sharpness", &sharpness, 0, 20))
            {
                SetTextureSharpness(sharpness);
            }
            ImGui::SameLine();
            UI::HelpMarker("Adjusts the sharpness of distant textures. Higher levels may cause shimmering or aliasing on certain textures.");
        }

        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Spacing();

        // Performance
        // -----------------------------------------------------------------------------------------------------
        if (ImGui::CollapsingHeader(ICON_MD_MONITOR_HEART " Performance", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Spacing();

            // toggle grass
            bool grassDisabled = g_config.disableGrass;
            if (ImGui::Checkbox("Disable Grass", &grassDisabled))
            {
                DisableGrass(grassDisabled);
            }
            ImGui::SameLine();
            UI::HelpMarker("Prevents grass patches from rendering, which will bring major performance improvements in areas with high foliage density.",
                "Disabling this requires reloading the current level before grass becomes visible again.");
        }

        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Spacing();

        // Restart Required
        // -----------------------------------------------------------------------------------------------------
        if (ImGui::CollapsingHeader(ICON_MD_DANGEROUS " Further options"))
        {
            ImGui::Spacing();

            UI::WarningText("These settings should be edited manually in the INI file and require an immediate game restart if changed here.", true);
            ImGui::Spacing();

            ImGui::SeparatorText("Graphics");
            ImGui::Spacing();


            // vsync
            bool vsync = g_config.vsync;
            if (ImGui::Checkbox("VSync", &vsync))
                EnableVSync(vsync);
            ImGui::SameLine();
            UI::HelpMarker(
                "Vertical Synchronisation (VSync) synchronizes the frame rate of the game with the refresh rate of your monitor to reduce screen tearing.",
                "Requires game restart");


            // render resolution
            bool renderRes = g_config.renderRes;
            if (ImGui::Checkbox("High-Res Rendering", &renderRes))
            {
                SetRenderRes(renderRes);
            }
            ImGui::SameLine();
            UI::HelpMarker("Allows the game to render at higher resolutions internally. Required for supersampling.",
                       "Requires game restart");


            // Supersampling
            ImGui::BeginDisabled(!g_config.renderRes);
            {
                const float ssLevels[] = {1.0f, 1.5f, 2.0f, 3.0f, 4.0f};
                const char* ssLabels[] = {"Off", "1.5x", "2x", "3x", "4x"};
                int currentSsIndex = 0;
                for (int i = 0; i < 5; i++)
                {
                    if (g_config.ssMultiplier >= ssLevels[i])
                        currentSsIndex = i;
                }
                if (ImGui::Combo("Supersampling", &currentSsIndex, ssLabels, 5))
                    SetSupersampling(ssLevels[currentSsIndex]);
                ImGui::SameLine();
                UI::HelpMarker(
                    "Renders the scene at a higher internal resolution and downscales. Higher values cost significantly more GPU.",
                    "Requires game restart");
            }
            ImGui::EndDisabled();


            ImGui::Spacing();
            ImGui::SeparatorText("Window");
            ImGui::Spacing();


            // windowed mode
            bool windowed = g_config.windowed;
            if (ImGui::Checkbox("Windowed Mode", &windowed))
                SetWindowed(windowed);
            ImGui::SameLine();
            UI::HelpMarker("Runs the game in a window instead of fullscreen.",
                       "Requires game restart");


            // borderless windowed mode
            ImGui::BeginDisabled(!g_config.windowed);
            {
                bool borderless = g_config.borderless;
                if (ImGui::Checkbox("Borderless", &borderless))
                    SetBorderless(borderless);
                ImGui::SameLine();
                UI::HelpMarker(
                    "Removes the window border and title bar when in windowed mode for a more seamless experience.",
                    "Requires game restart & windowed mode");
            }
            ImGui::EndDisabled();


            // resolution
            int resW = g_config.resolutionW;
            int resH = g_config.resolutionH;
            ImGui::PushItemWidth(ImGui::GetFontSize() * 5);
            if (ImGui::InputInt("##resW", &resW, 0, 0))
                SetResolutionW(resW);
            ImGui::SameLine();
            ImGui::Text("×");
            ImGui::SameLine();
            if (ImGui::InputInt("##resH", &resH, 0, 0))
                SetResolutionH(resH);
            ImGui::PopItemWidth();
            ImGui::SameLine();
            UI::HelpMarker("Custom resolution for the game. Set both width and height to 0 to use your desktop resolution.",
                       "Requires game restart");
        }

        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Spacing();

        // -- CREDITS --
        if (ImGui::CollapsingHeader(ICON_MD_INFO " Credits", ImGuiTreeNodeFlags_DefaultOpen))
        {
            //ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.98f, 0.26f, 0.26f, 1.00f)); // red color
            ImGui::TextWrapped("Skylanders Spyro's Adventure Improved");
            //ImGui::PopStyleColor();

            ImGui::Spacing();
            ImGui::Spacing();

            ImGui::Text("Developed by:");
            ImGui::SameLine();
            ImGui::TextLinkOpenURL("jacee", "https://github.com/j4ceee");

            ImGui::Spacing();
            ImGui::Spacing();

            ImGui::TextWrapped("Portal Emulation based on...");

            ImGui::Spacing();
            ImGui::Bullet();
            ImGui::SameLine();
            ImGui::TextLinkOpenURL("Cemu", "https://github.com/cemu-project/Cemu");

            ImGui::Spacing();
            ImGui::Bullet();
            ImGui::SameLine();
            ImGui::TextLinkOpenURL("RPCS3", "https://github.com/RPCS3/rpcs3");

            ImGui::Spacing();
            ImGui::Spacing();

            ImGui::TextWrapped("UI created with...");

            ImGui::Spacing();
            ImGui::Bullet();
            ImGui::SameLine();
            ImGui::TextLinkOpenURL("ImGui", "https://github.com/ocornut/imgui");

            ImGui::Spacing();
            ImGui::Bullet();
            ImGui::SameLine();
            ImGui::Text("Icons by Toys for Bob");

            ImGui::Spacing();
            ImGui::Bullet();
            ImGui::SameLine();
            ImGui::TextLinkOpenURL("Varela Round", "https://fonts.google.com/specimen/Varela+Round");
            ImGui::SameLine();
            ImGui::Text(" by The Varela Round Project Authors (SIL Open Font License, 1.1)");

            ImGui::Spacing();
            ImGui::Bullet();
            ImGui::SameLine();
            ImGui::TextLinkOpenURL("Material Design Icons", "https://github.com/google/material-design-icons/blob/master/font/MaterialIcons-Regular.ttf");
            ImGui::SameLine();
            ImGui::Text(" by Google (Apache 2.0 License)");

            ImGui::Spacing();
            ImGui::Bullet();
            ImGui::SameLine();
            ImGui::TextLinkOpenURL("IconFontCppHeaders", "https://github.com/juliettef/IconFontCppHeaders");
            ImGui::SameLine();
            ImGui::Text(" by Juliette Foucaut and Doug Binks (zlib License)");
        }
    }
} // namespace ssa::UIPages
