#pragma once
#include <codecvt>
#include <Windows.h>
#include <string>
#include "log.h"

namespace ssa
{
    struct Config
    {
        // Window
        bool windowed   = true; // run in windowed mode
        bool borderless = true; // borderless if in windowed mode
        int resolutionW = 0;    // 0 = use desktop resolution
        int resolutionH = 0;    // 0 = use desktop resolution

        // Graphics
        bool vsync          = true; // enable vertical sync
        int fpsCap          = 0;    // 0 = unlimited
        bool renderRes      = true; // should game render at chosen / native resolution internally?
        float ssMultiplier  = 1.0f; // supersampling: 1.0 = off, 1.5 = 1.5x, 2.0 = 2x SSAA
        int anisotropy      = 8;    // 1, 2, 4, 8, 16

        int textureSharpness= 10;   // 0 = off, 10 = default, 20 = max (maps to lodBias internally)
        float lodBias       = -1.0f;// derived from textureSharpness, do not set directly

        // Performance
        bool disableGrass   = false; // prevents grass patches from rendering

        // Mod
        LogLevel logLevel   = LogLevel::INFO; // log level (OFF, INFO, DEBUG, VERBOSE)
    };

    inline Config g_config;

    // set to false at startup if the config file cannot be written
    inline bool g_configWritable = true;

    // -------------------------------------------------------------------------
    // Path helper
    // -------------------------------------------------------------------------
    inline std::wstring GetConfigPath()
    {
        wchar_t path[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, path, MAX_PATH);
        // replace .exe with .ini
        std::wstring ws(path);
        auto pos = ws.rfind(L'\\');
        return (pos != std::wstring::npos ? ws.substr(0, pos + 1) : L"") + L"ssa_impr_mod.ini";
    }

    // -------------------------------------------------------------------------
    // Logging
    // -------------------------------------------------------------------------
    inline void LogSettings()
    {
        const char* level = nullptr;
        switch (g_config.logLevel) {
            case LogLevel::VERBOSE: level = "VERBOSE";  break;
            case LogLevel::DEBUG:   level = "DEBUG";    break;
            case LogLevel::INFO:    level = "INFO";     break;
            case LogLevel::OFF:     level = "OFF";      break;
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
        Log("[Config] Texture sharpness: %d (LOD bias: %.1f)", g_config.textureSharpness, g_config.lodBias);
        Log("[Config] Disable grass: %d", g_config.disableGrass);
    }

    // -------------------------------------------------------------------------
    // SaveConfig (full rewrite, preserves all comments)
    // called by every setter after updating g_config
    // -------------------------------------------------------------------------
    inline void SaveConfig()
    {
        if (!g_configWritable) return;

        std::wstring ini = GetConfigPath();
        FILE* f = nullptr;
        if (_wfopen_s(&f, ini.c_str(), L"w, ccs=UTF-8") != 0 || !f)
        {
            Log("[Config] SaveConfig failed: cannot open %ls for writing", ini.c_str());
            g_configWritable = false; // don't try again
            return;
        }

        fwprintf(f,
            L"; Skylanders Spyro's Adventure Improved - Mod Config\n"
            L"\n"
            L"[Window]\n"
            L"; Run game in windowed mode (0 = fullscreen, 1 = windowed (default))\n"
            L"Windowed=%d\n"
            L"; Remove window border / titlebar (requires Windowed=1) (0 = disabled, 1 = enabled (default))\n"
            L"Borderless=%d\n"
            L"; Custom resolution (set both to 0 to use your desktop resolution)\n"
            L"ResolutionW=%d\n"
            L"ResolutionH=%d\n"
            L"\n"
            L"[Graphics]\n"
            L"; Vertical sync (0 = disabled, 1 = enabled (default))\n"
            L"VSync=%d\n"
            L"; Frame rate cap. Set to 0 for unlimited.\n"
            L"FpsCap=%d\n"
            L"; Allows the game to render at higher resolutions internally (0 = disabled, 1 = enabled (default))\n"
            L"RenderRes=%d\n"
            L"; Supersampling multiplier (requires RenderRes=1) (1.0 = off (default), 1.5 = 1.5x SSAA, 2.0 = 2x SSAA)\n"
            L"; Multiplies the internal render resolution for improved image quality and scales the image down to your chosen / desktop resolution.\n"
            L"Supersampling=%.1f\n"
            L"; Anisotropic filtering level (1 = off, valid: 1/2/4/8/16)\n"
            L"Anisotropy=%d\n"
            L"; Texture sharpness (0 = off, 10 = default, 20 = maximum)\n"
            L"TextureSharpness=%d\n"
            L"\n"
            L"[Performance]\n"
            L"; Disable grass rendering (0 = grass is rendered (default), 1 = grass is not rendered)\n"
            L"; Disabling grass rendering brings major performance improvements in areas with high foliage density\n"
            L"DisableGrass=%d\n"
            L"\n"
            L"[Mod]\n"
            L"; Log level (0 = OFF, 1 = INFO, 2 = DEBUG, 3 = VERBOSE) - leave unchanged unless you need to debug an issue\n"
            L"LogLevel=%d\n",

            // Window
            (int)g_config.windowed,
            (int)g_config.borderless,
            g_config.resolutionW,
            g_config.resolutionH,
            // Graphics
            (int)g_config.vsync,
            g_config.fpsCap,
            (int)g_config.renderRes,
            g_config.ssMultiplier,
            g_config.anisotropy,
            g_config.textureSharpness,
            // Performance
            (int)g_config.disableGrass,
            // Mod
            (int)g_config.logLevel
        );

        fclose(f);
        Log("[Config] Saved config to %ls", ini.c_str());
    }

    // -------------------------------------------------------------------------
    // LoadConfig (read-only, called once at startup before EnsureConfigDefaults)
    // -------------------------------------------------------------------------
    inline void LoadConfig()
    {
        std::wstring ini = GetConfigPath();
        const wchar_t* f = ini.c_str();

        // -- Helpers
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

        // -- Config
        // Window
        g_config.windowed = getInt(L"Window", L"Windowed", 1) != 0;
        g_config.borderless = getInt(L"Window", L"Borderless", 1) != 0;
        g_config.resolutionW = getInt(L"Window", L"ResolutionW", 0);
        g_config.resolutionH = getInt(L"Window", L"ResolutionH", 0);

        // Graphics
        g_config.vsync = getInt(L"Graphics", L"VSync", 1) != 0;
        g_config.fpsCap = getInt(L"Graphics", L"FpsCap", 0);
        g_config.renderRes = getInt(L"Graphics", L"RenderRes", 1) != 0;

        g_config.ssMultiplier = std::max(1.0f, std::min(4.0f, getFloat(L"Graphics", L"Supersampling", 1.0f)));

        g_config.anisotropy = getInt(L"Graphics", L"Anisotropy", 8);
        g_config.anisotropy = std::max(1, std::min(16, g_config.anisotropy));

        int sharpness = getInt(L"Graphics", L"TextureSharpness", 10);
        sharpness = std::max(0, std::min(20, sharpness));
        g_config.textureSharpness = sharpness;
        g_config.lodBias = -(sharpness * 0.1f);

        // Performance
        g_config.disableGrass = getInt(L"Performance", L"DisableGrass", 0) != 0;

        // Mod
        int logLevelValue = getInt(L"Mod", L"LogLevel", 0);
        g_config.logLevel = static_cast<LogLevel>(std::max(0, std::min(3, logLevelValue)));

        Log("Config loaded from %ls", f);
        LogSettings();
    }

    // -------------------------------------------------------------------------
    // InitConfigFile (called once at startup after LoadConfig)
    // - creates the file if it does not exist
    // - rewrites it (filling any missing keys) if it does exist
    // - sets g_configWritable = false on failure so runtime saves are skipped
    // -------------------------------------------------------------------------
    inline bool InitConfigFile()
    {
        SaveConfig();

        if (!g_configWritable)
        {
            Log("[Config] EnsureConfigDefaults: cannot write — runtime config saves disabled");
            return false;
        }

        Log("[Config] Config ready at %ls", GetConfigPath().c_str());
        return true;
    }

    // -------------------------------------------------------------------------
    // Setters
    // -------------------------------------------------------------------------

    inline void SetFpsCap(int fpsValue)
    {
        g_config.fpsCap = std::max(0, fpsValue);
        SaveConfig();
    }

    inline void SetRenderRes(bool enabled)
    {
        g_config.renderRes = enabled;
        SaveConfig();
    }

    inline void SetSupersampling(float multiplier)
    {
        g_config.ssMultiplier = std::max(1.0f, std::min(4.0f, multiplier));
        SaveConfig();
    }

    inline void SetAnisotropy(int level)
    {
        // clamp to nearest valid level
        if (level <= 1) level = 1;
        else if (level <= 2) level = 2;
        else if (level <= 4) level = 4;
        else if (level <= 8) level = 8;
        else level = 16;
        g_config.anisotropy = level;
        SaveConfig();
    }

    inline void SetTextureSharpness(int sharpness)
    {
        sharpness = std::max(0, std::min(20, sharpness));
        g_config.textureSharpness = sharpness;
        g_config.lodBias = -(sharpness * 0.1f);
        SaveConfig();
    }

    inline void DisableGrass(bool value)
    {
        g_config.disableGrass = value;
        SaveConfig();
    }
} // namespace ssa
