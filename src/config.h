#pragma once
#include <Windows.h>
#include <string>
#include "log.h"

namespace ssa
{
    struct Config
    {
        LogLevel logLevel = LogLevel::INFO; // log level (OFF, INFO, DEBUG, VERBOSE)

        // Window
        bool windowed   = true; // run in windowed mode
        bool borderless = true; // borderless if in windowed mode
        int resolutionW = 0;    // 0 = use desktop resolution
        int resolutionH = 0;    // 0 = use desktop resolution

        // Graphics
        bool vsync = true; // enable vertical sync
        int fpsCap = 0; // 0 = unlimited
        float ssMultiplier = 1.0f; // supersampling: 1.0 = off, 1.5 = 1.5x, 2.0 = 2x SSAA
        int anisotropy = 8; // 1, 2, 4, 8, 16
        float lodBias = -1.0f; // texture sharpness: -1.5 = default, 0 = off, -2 = max

        bool renderRes = true; // should game render at chosen / native resolution internally?
    };

    inline Config g_config;

    inline std::wstring GetConfigPath()
    {
        wchar_t path[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, path, MAX_PATH);
        // replace .exe with .ini
        std::wstring ws(path);
        auto pos = ws.rfind(L'\\');
        return (pos != std::wstring::npos ? ws.substr(0, pos + 1) : L"") + L"ssa_impr_mod.ini";
    }

    inline void LogSettings()
    {
        const char* level = nullptr;
        switch (g_config.logLevel) {
        case LogLevel::VERBOSE:
            level = "VERBOSE";
            break;
        case LogLevel::DEBUG:
            level = "DEBUG";
            break;
        case LogLevel::INFO:
            level = "INFO";
            break;
        case LogLevel::OFF:
            level = "OFF";
            break;
        }

        Log("[Log] Log level: %s (%d)", level, g_config.logLevel);

        // log all settings:
        Log("[Config] Windowed: %d", g_config.windowed);
        Log("[Config] Borderless: %d", g_config.borderless);
        Log("[Config] ResolutionW: %d", g_config.resolutionW);
        Log("[Config] ResolutionH: %d", g_config.resolutionH);
        Log("[Config] RenderRes: %d", g_config.renderRes);
        Log("[Config] VSync: %d", g_config.vsync);
        Log("[Config] FpsCap: %d", g_config.fpsCap);
        Log("[Config] Supersampling multiplier: %.1f", g_config.ssMultiplier);
        Log("[Config] Anisotropy: %d", g_config.anisotropy);
        Log("[Config] Texture sharpness (LOD bias): %.1f", g_config.lodBias);
    }

    inline void LoadConfig()
    {
        std::wstring ini = GetConfigPath();
        const wchar_t* f = ini.c_str();

        auto getInt = [&](const wchar_t* sec, const wchar_t* key, int def)
        {
            return (int)GetPrivateProfileIntW(sec, key, def, f);
        };

        auto getFloat = [&](const wchar_t* sec, const wchar_t* key, float def) -> float
        {
            wchar_t buf[32] = {};
            GetPrivateProfileStringW(sec, key, nullptr, buf, 32, f);
            if (buf[0] == L'\0') return def;
            return wcstof(buf, nullptr);
        };

        g_config.windowed = getInt(L"Window", L"Windowed", 1) != 0;
        g_config.borderless = getInt(L"Window", L"Borderless", 1) != 0;
        g_config.resolutionW = getInt(L"Window", L"ResolutionW", 0);
        g_config.resolutionH = getInt(L"Window", L"ResolutionH", 0);

        g_config.vsync = getInt(L"Graphics", L"VSync", 1) != 0;
        g_config.fpsCap = getInt(L"Graphics", L"FpsCap", 0);
        g_config.renderRes = getInt(L"Graphics", L"RenderRes", 1) != 0;

        g_config.ssMultiplier = std::max(1.0f, std::min(4.0f, getFloat(L"Graphics", L"Supersampling", 1.0f)));

        g_config.anisotropy = getInt(L"Graphics", L"Anisotropy", 8);
        g_config.anisotropy = std::max(1, std::min(16, g_config.anisotropy));

        int sharpness = getInt(L"Graphics", L"TextureSharpness", 10);
        sharpness = std::max(0, std::min(20, sharpness));
        g_config.lodBias = -(sharpness * 0.1f);

        int logLevelValue = getInt(L"Mod", L"LogLevel", 0);
        g_config.logLevel = static_cast<LogLevel>(std::max(0, std::min(3, logLevelValue)));

        Log("Config loaded from %ls", f);
        LogSettings();
    }

    inline bool WriteDefaultConfig()
    {
        std::wstring ini = GetConfigPath();
        if (GetFileAttributesW(ini.c_str()) != INVALID_FILE_ATTRIBUTES) return true; // already exists, fine

        // write with comments directly
        FILE* f = nullptr;
        if (_wfopen_s(&f, ini.c_str(), L"w, ccs=UTF-8") != 0 || !f) return false; // no write access

        fwprintf(f,
            L"; Skylanders Spyro's Adventure Improved - Mod Config\n"
            L"\n"
            L"[Window]\n"
            L"; Run game in windowed mode (0 = fullscreen, 1 = windowed (default))\n"
            L"Windowed=1\n"
            L"; Remove window border / titlebar (requires Windowed=1) (0 = disabled, 1 = enabled (default))\n"
            L"Borderless=1\n"
            L"; Custom resolution (set both to 0 to use your desktop resolution)\n"
            L"ResolutionW=0\n"
            L"ResolutionH=0\n"
            L"\n"
            L"[Graphics]\n"
            L"; Vertical sync (0 = disabled, 1 = enabled (default))\n"
            L"VSync=1\n"
            L"; Frame rate cap. Set to 0 for unlimited.\n"
            L"FpsCap=0\n"
            L"; Allows the game to render at higher resolutions internally (0 = disabled, 1 = enabled (default))\n"
            L"RenderRes=1\n"
            L"; Supersampling multiplier (requires RenderRes=1) (1.0 = off (default), 1.5 = 1.5x SSAA, 2.0 = 2x SSAA)\n"
            L"; Multiplies the internal render resolution for improved image quality and scales the image down to your chosen / desktop resolution.\n"
            L"Supersampling=1.0\n"
            L"; Anisotropic filtering level (1 = off, valid: 1/2/4/8/16)\n"
            L"Anisotropy=8\n"
            L"; Texture sharpness (0 = off, 10 = default, 20 = maximum)\n"
            L"TextureSharpness=10\n"
            L"\n"
            L"[Mod]\n"
            L"; Log level (0 = OFF, 1 = INFO, 2 = DEBUG, 3 = VERBOSE) - leave unchanged unless you need to debug an issue\n"
            L"LogLevel=1\n"
        );
        fclose(f);
        Log("Wrote default config to %ls", ini.c_str());
        LogSettings();
        return true;
    }
} // namespace ssa
