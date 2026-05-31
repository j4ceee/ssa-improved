#include "ui.h"
#include "log.h"
#include "fonts/materialicons.cpp"
#include "fonts/varela_round_reg.cpp"
#include "fonts/skylanders_icons.cpp"
#include <imgui.h>

#include "config.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#include "imgui_internal.h"
#include "graphics/window_hooks.h"

namespace ssa
{

void UI::Initialize()
{
    if (m_initialized) {
        return;
    }

    Log("Initializing ImGui...");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    // io.IniFilename = "GZConsoleThingy.ini";

    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.MouseDrawCursor = false;

    SetupUiLook();

    // register custom settings handler
    // ImGuiSettingsHandler ini_handler;
    // ini_handler.TypeName = "GZConsoleThingy";
    // ini_handler.TypeHash = ImHashStr("GZConsoleThingy");
    // // ini_handler.ReadOpenFn = SettingsHandlerReadOpen;
    // // ini_handler.ReadLineFn = SettingsHandlerReadLine;
    // // ini_handler.WriteAllFn = SettingsHandlerWriteAll;
    // ini_handler.UserData = this;
    // ImGui::AddSettingsHandler(&ini_handler);

    m_initialized = true;
    Log("ImGui initialized successfully");
}

void UI::Shutdown()
{
    if (!m_initialized) {
        return;
    }

    Log("Shutting down ImGui");
    ImGui_ImplWin32_Shutdown();
    ImGui_ImplDX9_Shutdown();
    ImGui::DestroyContext();
    m_initialized = false;
}

void UI::SetupUiLook()
{
    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    ImGuiIO& io = ImGui::GetIO();

    // query DPI from the game window (or fallback to desktop DPI)
    float dpiScale = 1.0f;
    dpiScale = GetDeviceCaps(GetDC(nullptr), LOGPIXELSX) / 96.0f;
    float fontSize = std::round(16.0f * dpiScale); // e.g. 32px on 4K

    // fonts
    mainfont = io.Fonts->AddFontFromMemoryCompressedTTF(
        VarelaRoundReg_compressed_data,
        VarelaRoundReg_compressed_size,
        fontSize
    );

    ImFontConfig config;
    config.MergeMode = true;
    config.PixelSnapH = true;

    io.Fonts->AddFontFromMemoryCompressedTTF(
        SkylandersIcons_compressed_data,
        SkylandersIcons_compressed_size,
        fontSize,
        &config
    );

    io.Fonts->AddFontFromMemoryCompressedTTF(
        MaterialIcons_compressed_data,
        MaterialIcons_compressed_size,
        0.75f * fontSize,
        &config
    );

    // colours
    ImVec4* colors = style.Colors;

    // -- Text
    colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.52f, 1.00f);
    // -- Backgrounds
    colors[ImGuiCol_WindowBg]               = ImVec4(0.05f, 0.05f, 0.06f, 0.88f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.06f, 0.08f, 0.12f, 0.97f);
    // -- Borders
    colors[ImGuiCol_Border]                 = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    // -- Title bar
    colors[ImGuiCol_TitleBg]                = ImVec4(0.04f, 0.10f, 0.20f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.06f, 0.15f, 0.30f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.03f, 0.07f, 0.14f, 0.80f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.04f, 0.10f, 0.20f, 1.00f);
    // -- Scrollbar
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.05f, 0.05f, 0.06f, 0.50f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.16f, 0.28f, 0.47f, 1.00f); // sky-mid
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.24f, 0.36f, 0.56f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.35f, 0.54f, 0.84f, 1.00f);
    // -- Frames (InputText, Combo box, etc.)
    colors[ImGuiCol_FrameBg]                = ImVec4(0.14f, 0.14f, 0.15f, 0.70f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.16f, 0.28f, 0.47f, 0.30f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.16f, 0.28f, 0.47f, 0.50f);
    // -- Widgets
    colors[ImGuiCol_CheckMark]              = ImVec4(0.35f, 0.54f, 0.84f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.16f, 0.28f, 0.47f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.35f, 0.54f, 0.84f, 1.00f);
    // -- Buttons
    colors[ImGuiCol_Button]                 = ImVec4(0.10f, 0.20f, 0.40f, 0.90f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.24f, 0.36f, 0.56f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.35f, 0.54f, 0.84f, 1.00f);
    // -- Headers
    colors[ImGuiCol_Header]                 = ImVec4(0.16f, 0.28f, 0.47f, 0.20f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.16f, 0.28f, 0.47f, 0.55f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.24f, 0.36f, 0.56f, 0.90f);
    // -- Separators
    colors[ImGuiCol_Separator]              = ImVec4(0.35f, 0.35f, 0.36f, 0.50f);
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.35f, 0.54f, 0.84f, 0.65f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.35f, 0.54f, 0.84f, 1.00f);
    // -- Resize grip
    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.16f, 0.28f, 0.47f, 0.18f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.16f, 0.28f, 0.47f, 0.55f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.35f, 0.54f, 0.84f, 0.90f);
    // -- Tabs
    colors[ImGuiCol_Tab]                    = ImVec4(0.10f, 0.10f, 0.11f, 0.90f);
    colors[ImGuiCol_TabHovered]             = ImVec4(0.16f, 0.28f, 0.47f, 0.55f);
    colors[ImGuiCol_TabSelected]            = ImVec4(0.08f, 0.16f, 0.30f, 1.00f);
    colors[ImGuiCol_TabSelectedOverline]    = ImVec4(1.00f, 0.72f, 0.10f, 1.00f); // gold: only here
    colors[ImGuiCol_TabDimmed]              = ImVec4(0.06f, 0.06f, 0.07f, 0.97f);
    colors[ImGuiCol_TabDimmedSelected]      = ImVec4(0.06f, 0.10f, 0.18f, 1.00f);
    // -- Nav
    colors[ImGuiCol_NavHighlight]           = ImVec4(0.35f, 0.54f, 0.84f, 0.85f);
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 0.72f, 0.10f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.04f, 0.08f, 0.16f, 0.50f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.04f, 0.08f, 0.16f, 0.55f);

    style.WindowRounding    = 15.0f;
    style.FrameRounding     = 7.0f;
    style.GrabRounding      = 4.0f;
    style.TabRounding       = 4.0f;
    style.PopupRounding     = 6.0f;
    style.ScrollbarRounding = 4.0f;
    style.ChildRounding     = 5.0f;

    style.WindowBorderSize  = 3.0f;
    style.FrameBorderSize   = 0.0f;
    style.PopupBorderSize   = 3.0f;

    style.WindowPadding     = ImVec2(14.0f, 12.0f);
    style.FramePadding      = ImVec2( 8.0f,  5.0f);
    style.ItemSpacing       = ImVec2(10.0f,  7.0f);
    style.ItemInnerSpacing  = ImVec2( 7.0f,  5.0f);
    style.CellPadding       = ImVec2( 6.0f,  4.0f);
    style.IndentSpacing     = 20.0f;
    style.ScrollbarSize     = 12.0f;
    style.GrabMinSize       = 12.0f;

    style.ScaleAllSizes(0.75f * dpiScale); // scales padding, grab sizes, etc. proportionally

    // cursor
    io.MouseDrawCursor = false;

    // apply user font size
    io.FontGlobalScale = g_config.uiFontScale;
}

void UI::SetVisible(bool visible)
{
    m_visible = visible;

    if (visible) {
        WindowHooks::orig_ClipCursor(nullptr); // release mouse to whole desktop
        WindowHooks::orig_ShowCursor(TRUE); // ensure cursor is visible when alt-tabbed
    } else if (WindowHooks::g_hasFocus) {
        if (WindowHooks::g_hasClipRect)
            WindowHooks::orig_ClipCursor(&WindowHooks::g_gameClipRect);
        WindowHooks::orig_ShowCursor(FALSE);
    }
}

bool UI::HandleHotkeyCapture(WPARAM wParam)
{
    UI* ui = Get();

    if (!ui->m_isCapturingToggleUI) {
        return false;
    }

    if (!IsValidKey(static_cast<int>(wParam))) {
        return true;
    }

    if (ui->m_isCapturingToggleUI) {
        ui->m_isCapturingToggleUI = false;
        ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
        Log("Toggle UI key bound to: %s", GetKeyName(static_cast<int>(wParam)).c_str());
    }

    return true;
}

void UI::Render()
{
    if (!m_initialized) {
        return;
    }

    // --- CONTROLLER TOGGLE LOGIC ---
    // check for L3 + R3 (Left Stick Click + Right Stick Click)
    static bool s_wasComboPressed = false;
    bool isComboPressed = ImGui::IsKeyDown(ImGuiKey_GamepadL3) && ImGui::IsKeyDown(ImGuiKey_GamepadR3);

    // only trigger once per press to prevent flickering open / closed
    if (isComboPressed && !s_wasComboPressed) {
        SetVisible(!m_visible);
    }
    s_wasComboPressed = isComboPressed;
    // -------------------------------

    if (!m_visible) {
        return;
    }

    ImGui::Begin("Skylanders Spyro's Adventure Improved", nullptr, ImGuiWindowFlags_NoCollapse);
    ImGui::PushFont(mainfont);

    ImGui::Spacing();

    if (ImGui::BeginTabBar("##tabs", ImGuiTabBarFlags_None)) {
        if (ImGui::BeginTabItem("Portal")) {
            ImGui::BeginChild("PortalContent", ImVec2(0, 0), false);
            ImGui::Spacing();
            UIPages::RenderPortalTab();
            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Settings")) {
            ImGui::BeginChild("SettingsContent", ImVec2(0, 0), false);
            ImGui::Spacing();
            UIPages::RenderSettingsTab();
            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::PopFont();
    ImGui::End();


    if (m_creatorVisible)
    {
        ImGui::Begin("Skylander Creator", &m_creatorVisible);
        ImGui::PushFont(mainfont);

        ImGui::Spacing();

        UIPages::RenderCreator();

        ImGui::PopFont();
        ImGui::End();
    }
}

// Helper functions
void UI::HelpMarker(const char* desc, const char* warning)
{
    if (ImGui::BeginItemTooltip())
    {
        InfoText(desc, warning);
        ImGui::EndTooltip();
    }
    ImGui::TextDisabled("(?)");
    if (ImGui::BeginItemTooltip())
    {
        InfoText(desc, warning);
        ImGui::EndTooltip();
    }
}
// help marker with array of bullet points for warning
void UI::HelpMarker(const char* desc, const std::vector<const char*>& warnings)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::BeginItemTooltip())
    {
        InfoText(desc, warnings);
        ImGui::EndTooltip();
    }
}

void UI::HoverTooltip(const char* text, const char* warning)
{
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        InfoText(text, warning);
        ImGui::EndTooltip();
    }
}

void UI::InfoText(const char* text, const char* warning)
{
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
    ImGui::TextUnformatted(text);

    if (warning)
    {
        ImGui::Separator();
        WarningText(warning);
    }

    ImGui::PopTextWrapPos();
}
// info text with array of bullet points for warning
void UI::InfoText(const char* text, const std::vector<const char*>& warnings)
{
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
    ImGui::TextUnformatted(text);

    if (!warnings.empty())
    {
        ImGui::Separator();
        for (const char* warning : warnings)
        {
            StartWarningText();
            ImGui::Bullet();
            ImGui::SameLine();
            ImGui::Text("%s", warning);
            EndWarningText();
        }
    }
}

void UI::StartWarningText()
{
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.0f, 1.0f));
}

void UI::EndWarningText()
{
    ImGui::PopStyleColor();
}

void UI::WarningText(const char* text, bool wrap)
{
    StartWarningText();
    if (wrap)
    {
        ImGui::TextWrapped("%s", text);
    }
    else
    {
        ImGui::Text("%s", text);
    }
    EndWarningText();
}
} // namespace ssa