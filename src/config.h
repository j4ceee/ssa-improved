#pragma once
#include <Windows.h>
#include <filesystem>
#include <string>
#include <imgui.h>
#include "log.h"

namespace ssa
{
    namespace fs = std::filesystem;

    struct Config
    {
        // Window
        bool windowed = true; // run in windowed mode
        bool borderless = true; // borderless if in windowed mode
        int resolutionW = 0; // 0 = use desktop resolution
        int resolutionH = 0; // 0 = use desktop resolution

        // Graphics
        bool vsync = true; // enable vertical sync
        int fpsCap = 0; // 0 = unlimited
        bool renderRes = true; // should game render at chosen / native resolution internally?
        float ssMultiplier = 1.0f; // supersampling: 1.0 = off, 1.5 = 1.5x, 2.0 = 2x SSAA
        int anisotropy = 8; // 1, 2, 4, 8, 16

        int textureSharpness = 10; // 0 = off, 10 = default, 20 = max (maps to lodBias internally)
        float lodBias = -1.0f; // derived from textureSharpness, do not set directly

        // Performance
        bool disableGrass = false; // prevents grass patches from rendering

        // Game
        bool emulatedPortal = false; // if true, the physical portal is ignored and all portal interactions are emulated in software (see portal/backend/EmulatedBackend.h)
        bool emulatedPortalStartup = false; // value of emulated portal during startup (should be used everywhere where emulated portal needs to be checked)

        // Mod
        float uiFontScale = 1.0f;
        bool textureMods = true; // enable texture mods
        bool textureDump = false; // dump all in-game textures to disk (for modders)
        LogLevel logLevel = LogLevel::INFO; // log level (OFF, INFO, DEBUG, VERBOSE)
    };

    inline Config g_config;

    // set to false at startup if the config file cannot be written
    inline bool g_configWritable = true;

    // -------------------------------------------------------------------------
    // Path helper
    // -------------------------------------------------------------------------
    inline std::wstring GetConfigPath()
    {
        wchar_t modulePath[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, modulePath, MAX_PATH);

        fs::path configPath = fs::path(modulePath).parent_path() / "ssa-improved" / "ssa_impr_mod.ini";

        fs::create_directories(configPath.parent_path());
        return configPath.wstring();
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

        LogF("[Log] Log level: %s (%d)", level, g_config.logLevel);
        // log all settings:
        LogF("[Config] Windowed: %d", g_config.windowed);
        LogF("[Config] Borderless: %d", g_config.borderless);
        LogF("[Config] ResolutionW: %d", g_config.resolutionW);
        LogF("[Config] ResolutionH: %d", g_config.resolutionH);
        LogF("[Config] RenderRes: %d", g_config.renderRes);
        LogF("[Config] VSync: %d", g_config.vsync);
        LogF("[Config] FpsCap: %d", g_config.fpsCap);
        LogF("[Config] Supersampling multiplier: %.1f", g_config.ssMultiplier);
        LogF("[Config] Texture sharpness: %d (LOD bias: %.1f)", g_config.textureSharpness, g_config.lodBias);
        LogF("[Config] Disable grass: %d", g_config.disableGrass);
        LogF("[Config] Emulated portal: %d", g_config.emulatedPortal);
        LogF("[Config] Font scale: %.1f", g_config.uiFontScale);
        LogF("[Config] Texture mods: %d", g_config.textureMods);
        LogF("[Config] Texture dump: %d", g_config.textureDump);
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
            L"[Game]\n"
            L"; Use a fully emulated portal instead of a physical USB device (0 = disabled (default), 1 = enabled)\n"
            L"; When enabled, all other portal backends are ignored. Manage figures via the Portal tab in the mod menu.\n"
            L"EmulatedPortal=%d\n"
            L"\n"
            L"[Mod]\n"
            L"; Scale of the font of the in-game UI (1.0 = default size, 2.0 = double size, etc.)\n"
            L"FontScale=%.1f\n"
            L"; Enable texture mods (0 = disabled, 1 = enabled (default))\n"
            L"TextureMods=%d\n"
            L"; Dump in-game textures to disk for modding purposes (0 = disabled (default), 1 = enabled)\n"
            L"; Warning: for mod creators only, this will write lots of files!\n"
            L"TextureDump=%d\n"
            L"; Log level (0 = OFF, 1 = INFO, 2 = DEBUG, 3 = VERBOSE) - leave unchanged unless you need to debug an issue\n"
            L"LogLevel=%d\n",

            // Window
            static_cast<int>(g_config.windowed),
            static_cast<int>(g_config.borderless),
            g_config.resolutionW,
            g_config.resolutionH,

            // Graphics
            static_cast<int>(g_config.vsync),
            g_config.fpsCap,
            static_cast<int>(g_config.renderRes),
            g_config.ssMultiplier,
            g_config.anisotropy,
            g_config.textureSharpness,

            // Performance
            static_cast<int>(g_config.disableGrass),

            // Game
            static_cast<int>(g_config.emulatedPortal),

            // Mod
            g_config.uiFontScale,
            static_cast<int>(g_config.textureMods),
            static_cast<int>(g_config.textureDump),
            static_cast<int>(g_config.logLevel)
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

        int anisotropy = getInt(L"Graphics", L"Anisotropy", 8);
        // clamp to nearest valid level
        if (anisotropy <= 1) anisotropy = 1;
        else if (anisotropy <= 2) anisotropy = 2;
        else if (anisotropy <= 4) anisotropy = 4;
        else if (anisotropy <= 8) anisotropy = 8;
        else anisotropy = 16;
        g_config.anisotropy = anisotropy;

        int sharpness = getInt(L"Graphics", L"TextureSharpness", 10);
        sharpness = std::max(0, std::min(20, sharpness));
        g_config.textureSharpness = sharpness;
        g_config.lodBias = -(sharpness * 0.1f);

        // Performance
        g_config.disableGrass = getInt(L"Performance", L"DisableGrass", 0) != 0;

        // Game
        g_config.emulatedPortal = getInt(L"Game", L"EmulatedPortal", 0) != 0;
        g_config.emulatedPortalStartup = g_config.emulatedPortal; // cache initial value

        // Mod
        float fontScale = getFloat(L"Mod", L"FontScale", 1.0f);
        g_config.uiFontScale = std::max(0.5f, std::min(3.0f, fontScale));

        g_config.textureMods = getInt(L"Mod", L"TextureMods", 1);
        g_config.textureDump = getInt(L"Mod", L"TextureDump", 0);

        int logLevelValue = getInt(L"Mod", L"LogLevel", static_cast<int>(LogLevel::INFO));
        g_config.logLevel = static_cast<LogLevel>(std::max(0, std::min(3, logLevelValue)));

        LogF("Config loaded from %ls", f);
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
            Log("[Config] EnsureConfigDefaults: cannot write - runtime config saves disabled");
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

    inline void EnableVSync(bool value)
    {
        g_config.vsync = value;
        SaveConfig();
    }

    inline void SetWindowed(bool value)
    {
        g_config.windowed = value;
        SaveConfig();
    }

    inline void SetBorderless(bool value)
    {
        g_config.borderless = value;
        SaveConfig();
    }

    inline void SetResolutionW(int value)
    {
        g_config.resolutionW = std::max(0, value);
        SaveConfig();
    }

    inline void SetResolutionH(int value)
    {
        g_config.resolutionH = std::max(0, value);
        SaveConfig();
    }

    inline void SetEmulatedPortal(bool value)
    {
        g_config.emulatedPortal = value;
        SaveConfig();
    }

    inline void SetFontScale(float value)
    {
        g_config.uiFontScale = std::max(0.5f, std::min(3.0f, value));
        ImGui::GetIO().FontGlobalScale = g_config.uiFontScale;
        SaveConfig();
    }

    inline void EnableTextureMods(bool value)
    {
        g_config.textureMods = value;
        SaveConfig();
    }

    inline void EnableTextureDump(bool value)
    {
        g_config.textureDump = value;
        SaveConfig();
    }

} // namespace ssa
