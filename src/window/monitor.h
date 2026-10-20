#pragma once
#include <Windows.h>
#include <cstdlib>
#include <cstring>
#include <vector>
#include "config.h"
#include "log.h"

namespace ssa::Monitor
{
    struct Info
    {
        HMONITOR handle;
        RECT rect; // full monitor area in virtual-desktop coordinates (can be negative)
        bool primary;
        int number; // N of \\.\DISPLAYN, 0 if unparsable
    };

    struct Target
    {
        HMONITOR handle = nullptr;
        RECT rect = {};
        int width() const { return rect.right - rect.left; }
        int height() const { return rect.bottom - rect.top; }
    };

    inline std::vector<Info> Enumerate()
    {
        std::vector<Info> out;
        EnumDisplayMonitors(nullptr, nullptr, [](HMONITOR h, HDC, LPRECT, LPARAM lp) -> BOOL
        {
            MONITORINFOEXA mi = {};
            mi.cbSize = sizeof(mi);
            if (!GetMonitorInfoA(h, &mi)) return TRUE;

            const char* p = strstr(mi.szDevice, "DISPLAY"); // "\\.\DISPLAY2"
            reinterpret_cast<std::vector<Info>*>(lp)->push_back({
                h, mi.rcMonitor, (mi.dwFlags & MONITORINFOF_PRIMARY) != 0, p ? atoi(p + 7) : 0
            });
            return TRUE;
        }, reinterpret_cast<LPARAM>(&out));
        return out;
    }

    // resolved once, so window creation, SetWindowPos and every Reset agree on the same monitor
    inline const Target& Get()
    {
        static Target target;
        static bool resolved = false;
        if (resolved) return target;
        resolved = true;

        const auto monitors = Enumerate();
        const Info* pick = nullptr;

        if (g_config.monitor > 0)
        {
            for (const auto& m : monitors)
                if (m.number == g_config.monitor)
                {
                    pick = &m;
                    break;
                }
            if (!pick)
                Log("[Window] Monitor %d not found, falling back to primary", g_config.monitor);
        }
        if (!pick)
            for (const auto& m : monitors)
                if (m.primary)
                {
                    pick = &m;
                    break;
                }

        if (pick)
        {
            target.handle = pick->handle;
            target.rect = pick->rect;
        }
        else
        {
            // enumeration failed: old behaviour
            target.rect = {0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)};
        }

        Log("[Window] Target monitor: (%d,%d) %dx%d", target.rect.left, target.rect.top, target.width(),
            target.height());
        return target;
    }
}
