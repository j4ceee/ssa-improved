#pragma once

#include <string>
#include <vector>
#include "imgui.h"
#include <Windows.h>
#include "imgui_internal.h"

namespace ssa
{

    class UI
    {
    public:
        static UI* Get()
        {
            static UI instance;
            return &instance;
        }

        void Initialize();
        void SetupUiLook();
        void Shutdown();
        void Render();

        bool IsVisible() const { return m_visible; }
        void SetVisible(bool visible);
        void ToggleVisible() { SetVisible(!m_visible); }

        bool IsCreatorVisible() { return m_creatorVisible; }
        void SetCreatorVisible(bool visible) { m_creatorVisible = visible; }
        void ToggleCreatorVisible() { SetCreatorVisible(!m_creatorVisible); }

        static bool HandleHotkeyCapture(WPARAM wParam);

        // helper functions for tabs to use
        static void HelpMarker(const char* desc, const char* warning = nullptr);
        static void HelpMarker(const char* desc, const std::vector<const char*>& warnings);
        static void HoverTooltip(const char* text, const char* warning = nullptr);
        static void InfoText(const char* text, const char* warning = nullptr);
        static void InfoText(const char* text, const std::vector<const char*>& warnings);
        static void StartWarningText();
        static void EndWarningText();
        static void WarningText(const char* text, bool wrap = false);
        static void StartDisabledText() { ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]); }
        static void EndDisabledText() { ImGui::PopStyleColor(); }


        bool m_isCapturingToggleUI = false;
    private:
        UI() = default;
        ~UI() = default;

        bool m_visible = false;
        bool m_creatorVisible = false;
        bool m_initialized = false;
        ImFont* mainfont = nullptr;

        // ImGui settings handlers
        // static void* SettingsHandlerReadOpen(ImGuiContext*, ImGuiSettingsHandler*, const char* name);
        // static void SettingsHandlerReadLine(ImGuiContext*, ImGuiSettingsHandler*, void* entry, const char* line);
        // static void SettingsHandlerWriteAll(ImGuiContext*, ImGuiSettingsHandler*, ImGuiTextBuffer* buf);

        static std::string GetKeyName(int vkCode)
        {
            char keyName[128];
            UINT scanCode = MapVirtualKeyA(vkCode, MAPVK_VK_TO_VSC);

            // for extended keys, set the extended key flag
            LONG lParam = scanCode << 16;
            if (vkCode == VK_INSERT || vkCode == VK_DELETE ||
                vkCode == VK_HOME || vkCode == VK_END ||
                vkCode == VK_PRIOR || vkCode == VK_NEXT)
            {
                lParam |= (1 << 24);
            }

            if (GetKeyNameTextA(lParam, keyName, sizeof(keyName)) > 0) {
                return std::string(keyName);
            }

            return "Unknown";
        }

        static bool IsValidKey(int vkCode)
        {
            // exclude mouse buttons
            if (vkCode >= VK_LBUTTON && vkCode <= VK_XBUTTON2) {
                return false;
            }

            // exclude system keys
            if (vkCode == VK_ESCAPE || vkCode == VK_TAB ||
                vkCode == VK_RETURN || vkCode == VK_BACK) {
                return false;
                }

            // exclude modifier-only keys
            if (vkCode == VK_SHIFT || vkCode == VK_CONTROL ||
                vkCode == VK_MENU || vkCode == VK_LWIN || vkCode == VK_RWIN) {
                return false;
                }

            // accept function keys (F1-F24)
            if (vkCode >= VK_F1 && vkCode <= VK_F24) {
                return true;
            }

            // accept alphanumeric keys (0-9, A-Z)
            if ((vkCode >= '0' && vkCode <= '9') || (vkCode >= 'A' && vkCode <= 'Z')) {
                return true;
            }

            // accept navigation keys
            if (vkCode >= VK_PRIOR && vkCode <= VK_DOWN) {
                return true;
            }

            // accept Insert, Delete
            if (vkCode == VK_INSERT || vkCode == VK_DELETE) {
                return true;
            }

            // accept numpad keys
            if (vkCode >= VK_NUMPAD0 && vkCode <= VK_DIVIDE)
            {
                return true;
            }

            // accept other common keys
            switch (vkCode) {
            case VK_OEM_1:      // ;
            case VK_OEM_PLUS:   // +
            case VK_OEM_COMMA:  // ,
            case VK_OEM_MINUS:  // -
            case VK_OEM_PERIOD: // .
            case VK_OEM_2:      // /
            case VK_OEM_3:      // ~
            case VK_OEM_4:      // [
            case VK_OEM_5:      // \|
            case VK_OEM_6:      // ]
            case VK_OEM_7:      // '
                return true;
            default:
                break;
            }

            return false;
        }
    };

    // page rendering functions (implemented in separate files)
    namespace UIPages
    {
        void RenderSettingsTab();
        void RenderGameTab();
        void RenderPortalTab();

        void RenderCreator();
    }
} // namespace ssa