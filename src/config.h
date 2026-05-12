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
        int anisotropy = 8; // 1, 2, 4, 8, 16
        float lodBias = -1.5f; // texture sharpness: -1.5 = default, 0 = off, -2 = max
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

    inline void CommunicateLogLevel()
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

        Log("[Log] Log level set to %s (%d)", level, g_config.logLevel);
    }

    inline void LoadConfig()
    {
        std::wstring ini = GetConfigPath();
        const wchar_t* f = ini.c_str();

        auto getInt = [&](const wchar_t* sec, const wchar_t* key, int def)
        {
            return (int)GetPrivateProfileIntW(sec, key, def, f);
        };

        g_config.windowed = getInt(L"Window", L"Windowed", 1) != 0;
        g_config.borderless = getInt(L"Window", L"Borderless", 1) != 0;
        g_config.resolutionW = getInt(L"Window", L"ResolutionW", 0);
        g_config.resolutionH = getInt(L"Window", L"ResolutionH", 0);

        g_config.vsync = getInt(L"Graphics", L"VSync", 1) != 0;
        g_config.fpsCap = getInt(L"Graphics", L"FpsCap", 0);

        g_config.anisotropy = getInt(L"Graphics", L"Anisotropy", 8);
        g_config.anisotropy = std::max(1, std::min(16, g_config.anisotropy));

        int sharpness = getInt(L"Graphics", L"TextureSharpness", 15);
        sharpness = std::max(0, std::min(20, sharpness));
        g_config.lodBias = -(sharpness * 0.1f);

        int logLevelValue = getInt(L"Mod", L"LogLevel", 0);
        g_config.logLevel = static_cast<LogLevel>(std::max(0, std::min(3, logLevelValue)));

        Log("Config loaded from %ls", f);
        CommunicateLogLevel();
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
            L"; Run game in windowed mode (0 = fullscreen, 1 = windowed)\n"
            L"Windowed=1\n"
            L"; Remove window border / titlebar (windowed mode needs to be active) (0 = disabled, 1 = enabled)\n"
            L"Borderless=1\n"
            L"; Custom resolution. Set both to 0 to use your desktop resolution.\n"
            L"ResolutionW=0\n"
            L"ResolutionH=0\n"
            L"\n"
            L"[Graphics]\n"
            L"; Vertical sync (0 = disabled, 1 = enabled)\n"
            L"VSync=1\n"
            L"; Frame rate cap. Set to 0 for unlimited.\n"
            L"FpsCap=0\n"
            L"; Anisotropic filtering level (1 = off, valid: 1/2/4/8/16)\n"
            L"Anisotropy=8\n"
            L"; Texture sharpness (0 = off, 15 = default, 20 = maximum)\n"
            L"TextureSharpness=15\n"
            L"\n"
            L"[Mod]\n"
            L"; Log level (0 = OFF, 1 = INFO, 2 = DEBUG, 3 = VERBOSE) - leave unchanged unless you need to debug an issue\n"
            L"LogLevel=1\n"
        );
        fclose(f);
        Log("Wrote default config to %ls", ini.c_str());
        CommunicateLogLevel();
        return true;
    }
} // namespace ssa
