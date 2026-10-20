#pragma once
#include <Windows.h>
#include "config.h"
#include "log.h"
#include "MinHook.h"
#include "imgui/ui.h"
#include "imgui.h"
#include "monitor.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace ssa::WindowHooks
{
    using CreateWindowExA_t         = HWND(WINAPI*)(DWORD, LPCSTR, LPCSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID);
    using SetWindowPos_t            = BOOL(WINAPI*)(HWND, HWND, int, int, int, int, UINT);
    using ChangeDisplaySettingsA_t  = LONG(WINAPI*)(DEVMODEA*, DWORD);
    using CreateWindowExW_t         = HWND(WINAPI*)(DWORD, LPCWSTR, LPCWSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID);
    using ChangeDisplaySettingsW_t  = LONG(WINAPI*)(DEVMODEW*, DWORD);
    using ShowWindow_t              = BOOL(WINAPI*)(HWND, int);
    using ClipCursor_t              = BOOL(WINAPI*)(const RECT*);
    using ShowCursor_t              = int(WINAPI*)(BOOL);
    using SetWindowLongA_t          = LONG(WINAPI*)(HWND, int, LONG);
    using SetWindowLongW_t          = LONG(WINAPI*)(HWND, int, LONG);
    using GetKeyState_t             = SHORT(WINAPI*)(int);
    using GetCursorPos_t            = BOOL(WINAPI*)(LPPOINT);
    using SetCursorPos_t            = BOOL(WINAPI*)(int, int);
    using GetSystemMetrics_t        = int(WINAPI*)(int);

    inline CreateWindowExA_t        orig_CreateWindowExA        = nullptr;
    inline SetWindowPos_t           orig_SetWindowPos           = nullptr;
    inline ChangeDisplaySettingsA_t orig_ChangeDisplaySettingsA = nullptr;
    inline CreateWindowExW_t        orig_CreateWindowExW        = nullptr;
    inline ChangeDisplaySettingsW_t orig_ChangeDisplaySettingsW = nullptr;
    inline ShowWindow_t             orig_ShowWindow             = nullptr;
    inline ClipCursor_t             orig_ClipCursor             = nullptr;
    inline ShowCursor_t             orig_ShowCursor             = nullptr;
    inline SetWindowLongA_t         orig_SetWindowLongA         = nullptr;
    inline SetWindowLongW_t         orig_SetWindowLongW         = nullptr;
    inline GetKeyState_t            orig_GetKeyState            = nullptr;
    inline GetCursorPos_t           orig_GetCursorPos           = nullptr;
    inline SetCursorPos_t           orig_SetCursorPos           = nullptr;
    inline GetSystemMetrics_t       orig_GetSystemMetrics       = nullptr;

    inline WNDPROC                  g_origWndProc               = nullptr;
    inline HHOOK                    g_hCBTHook                  = nullptr;

    inline bool g_hasFocus      = true;
    inline RECT g_gameClipRect  = {};
    inline bool g_hasClipRect   = false;
    inline POINT g_frozenCursorPos = {};

    // the game's main window HWND (populated in TrySetupGameWindow)
    inline HWND g_hGameWindow = nullptr;

    inline bool IsMainGameWindow(HWND hWndParent, DWORD dwStyle, int nWidth, int nHeight)
    {
        // must be a top-level window
        if (hWndParent != nullptr) return false;

        // must look like a real app window, not a utility/internal window
        bool looksLikeAppWindow = (dwStyle & (WS_OVERLAPPEDWINDOW | WS_POPUP)) != 0;
        if (!looksLikeAppWindow) return false;

        // must have non-trivial size (filters out D3D dummy windows, IME windows, etc.)
        // require at least 640x480 to be considered a candidate
        if (nWidth < 640 || nHeight < 480) return false;

        return true;
    }

    // the game's main window is created by NGL with style WS_OVERLAPPEDWINDOW and size 0x0 (resized afterwards),
    // which IsMainGameWindow rejects, so it is recognised by its window class
    inline bool IsGameWindowClass(LPCSTR className)
    {
        return !IS_INTRESOURCE(className) && strcmp(className, "NGL") == 0;
    }

    inline BOOL WINAPI hook_ClipCursor(const RECT* pRect)
    {
        if (pRect)
        {
            // game is setting a clip rect - save it but only apply if we have focus
            g_gameClipRect = *pRect;
            g_hasClipRect = true;
            if (!g_hasFocus || UI::Get()->IsVisible())
            {
                LogDebug("[Window] Deferred ClipCursor (no focus)");
                return TRUE;
            }
        }
        else
        {
            // game is releasing the clip - honour it and clear ours
            g_hasClipRect = false;
        }
        return orig_ClipCursor(pRect);
    }

    inline int WINAPI hook_ShowCursor(BOOL bShow)
    {
        // if we don't have focus or the ui is open, always show the cursor regardless of what the game wants
        if ((!g_hasFocus || UI::Get()->IsVisible()) && !bShow)
        {
            LogDebug("[Window] Suppressed ShowCursor(FALSE) - no focus");
            return 0; // return a plausible cursor count without actually hiding
        }
        return orig_ShowCursor(bShow);
    }

    inline BOOL WINAPI hook_ShowWindow(HWND hWnd, int nCmdShow)
    {
        if (g_config.borderless && hWnd == g_hGameWindow && nCmdShow == SW_MINIMIZE)
        {
            LogDebug("[Window] Suppressed ShowWindow(SW_MINIMIZE)");
            return TRUE;
        }
        return orig_ShowWindow(hWnd, nCmdShow);
    }

    inline LRESULT CALLBACK GameWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
        case WM_ACTIVATE:
            if (LOWORD(wParam) == WA_INACTIVE)
            {
                g_hasFocus = false;
                if (!UI::Get()->IsVisible()) // when UI is open the cursor is already released & visible
                {
                    orig_ClipCursor(nullptr); // release mouse to whole desktop
                    orig_ShowCursor(TRUE); // ensure cursor is visible when alt-tabbed
                }
                Log("[Window] Suppressed WM_ACTIVATE(WA_INACTIVE), released ClipCursor");
                return 0;
            }
            else
            {
                g_hasFocus = true;
                if (!UI::Get()->IsVisible())
                // when UI is open and focus is shifted into the game, don't hide mouse, we need it for the UI (UI will hide it)
                {
                    // if imgui ui not visible restore game clipping
                    if (g_hasClipRect)
                        orig_ClipCursor(&g_gameClipRect);
                    orig_ShowCursor(FALSE);
                }
                LogDebug("[Window] WM_ACTIVATE focus gained, restored ClipCursor");
            }
            break;

        case WM_ACTIVATEAPP:
            if (!wParam)
            {
                // already handled by WM_ACTIVATE above, just suppress
                Log("[Window] Suppressed WM_ACTIVATEAPP focus-loss");
                return 0;
            }
            break;

        case WM_SIZE:
            // windows can minimize us at the OS level bypassing WndProc entirely, catch resulting WM_SIZE & restore
            if (wParam == SIZE_MINIMIZED)
            {
                Log("[Window] Caught SIZE_MINIMIZED - restoring");
                orig_ShowWindow(hWnd, SW_RESTORE);
                return 0;
            }
            break;

        case WM_SYSCOMMAND:
            if ((wParam & 0xFFF0) == SC_MINIMIZE)
            {
                Log("[Window] Suppressed SC_MINIMIZE");
                return 0;
            }
            break;

        default:
            break;
        }

        // SAFETY CHECK: don't do anything ImGui-related until the context exists
        if (ImGui::GetCurrentContext() != nullptr)
        {
            if (uMsg == WM_KEYDOWN && wParam == VK_F1 && (lParam >> 30) == 0)
            {
                UI::Get()->ToggleVisible();
                return true;
            }

            if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
                return true;

            if (UI::Get()->IsVisible())
            {
                if (uMsg == WM_KEYDOWN && (lParam >> 30) == 0)
                    if (UI::HandleHotkeyCapture(wParam)) return true;
                return true;
            }
        }

        return CallWindowProcA(g_origWndProc, hWnd, uMsg, wParam, lParam);
    }

    inline LRESULT CALLBACK CBTProc(int nCode, WPARAM wParam, LPARAM lParam)
    {
        if (nCode == HCBT_MINMAX)
        {
            HWND hWnd = (HWND)wParam;
            int cmd = LOWORD(lParam);
            if (hWnd == g_hGameWindow &&
                (cmd == SW_MINIMIZE || cmd == SW_SHOWMINIMIZED
                    || cmd == SW_SHOWMINNOACTIVE || cmd == SW_FORCEMINIMIZE))
            {
                LogDebug("[Window] CBT blocked minimize");
                return 1; // non-zero cancels the action
            }
        }
        return CallNextHookEx(g_hCBTHook, nCode, wParam, lParam);
    }

    // called from hook_Present on the first frame
    // finds the real game window regardless of how/when/which thread it was created on
    inline void TrySetupGameWindow()
    {
        // walk top-level windows belonging to our process
        struct FindCtx
        {
            HWND result;
            DWORD pid;
        };
        FindCtx ctx = {nullptr, GetCurrentProcessId()};

        EnumWindows([](HWND hWnd, LPARAM lParam) -> BOOL
        {
            auto* ctx = reinterpret_cast<FindCtx*>(lParam);
            DWORD pid = 0;
            GetWindowThreadProcessId(hWnd, &pid);
            if (pid != ctx->pid) return TRUE;

            LONG style = GetWindowLongA(hWnd, GWL_STYLE);
            RECT r = {};
            GetWindowRect(hWnd, &r);
            int w = r.right - r.left;
            int h = r.bottom - r.top;

            // must be visible, sizeable / popup, and large enough to be the game
            if ((style & WS_VISIBLE) &&
                (style & (WS_POPUP | WS_THICKFRAME | WS_OVERLAPPEDWINDOW)) &&
                w >= 640 && h >= 480)
            {
                ctx->result = hWnd;
                return FALSE; // stop enumeration
            }
            return TRUE;
        }, (LPARAM)&ctx);

        if (!ctx.result) return;

        g_hGameWindow = ctx.result;
        Log("[Window] Found game window: HWND=%p", g_hGameWindow);

        // subclass the WndProc
        g_origWndProc = (WNDPROC)SetWindowLongPtrA(
            g_hGameWindow, GWLP_WNDPROC, (LONG_PTR)GameWndProc);
        Log("[Window] Subclassed game WndProc");

        // install CBT hook on the window's message thread
        DWORD tid = GetWindowThreadProcessId(g_hGameWindow, nullptr);
        g_hCBTHook = SetWindowsHookExA(WH_CBT, CBTProc, nullptr, tid);
        if (g_hCBTHook)
            Log("[Window] CBT hook installed on thread %u", tid);
        else
            Log("[Window] CBT hook failed: %d", GetLastError());
    }

    // DPI of the monitor the window is on (96 = 100% scaling).
    // GetDpiForWindow is looked up dynamically; falls back to the screen DC (= system DPI)
    inline UINT GetWindowDpi(HWND hWnd)
    {
        if (hWnd)
        {
            const auto dpiForWindow = reinterpret_cast<UINT(WINAPI*)(HWND)>(
                GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow"));
            if (dpiForWindow)
            {
                if (const UINT dpi = dpiForWindow(hWnd))
                    return dpi;
            }
        }

        UINT dpi = 96;
        if (HDC dc = GetDC(nullptr))
        {
            dpi = static_cast<UINT>(GetDeviceCaps(dc, LOGPIXELSX));
            ReleaseDC(nullptr, dc);
        }
        return dpi;
    }

    // one-off snapshot of everything that decides where / how large the game window is and how Windows DPI-scales it
    // (multi-monitor / mixed DPI debugging, called through D3D9Hooks::LogWindowDiagnostics)
    inline void LogWindowState(const char* tag, HWND focusWindow)
    {
        if (!g_hGameWindow)
        {
            LogDebug("[WinDiag:%s] no game window yet", tag);
            return;
        }
        const HWND hWnd = g_hGameWindow;

        // window geometry
        RECT wr = {}, cr = {};
        GetWindowRect(hWnd, &wr);
        GetClientRect(hWnd, &cr);
        LogDebug(
            "[WinDiag:%s] window (%ld,%ld) %ldx%ld, client %ldx%ld, style 0x%08lX, game window %p, focus window %p (%s)",
            tag, wr.left, wr.top, wr.right - wr.left, wr.bottom - wr.top, cr.right - cr.left, cr.bottom - cr.top,
            static_cast<unsigned long>(GetWindowLongA(hWnd, GWL_STYLE)), static_cast<void*>(hWnd),
            static_cast<void*>(focusWindow),
            focusWindow == hWnd ? "same" : "DIFFERENT");

        // monitor the window is actually on
        const HMONITOR mon = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFOEXA mi = {};
        mi.cbSize = sizeof(mi);
        if (GetMonitorInfoA(mon, &mi))
            LogDebug("[WinDiag:%s] window is on %s: (%ld,%ld) %ldx%ld%s", tag, mi.szDevice, mi.rcMonitor.left,
                     mi.rcMonitor.top,
                     mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top,
                     (mi.dwFlags & MONITORINFOF_PRIMARY) ? " (primary)" : "");

        // DPI (newer APIs are looked up dynamically so this builds against any SDK target)
        const HMODULE user32 = GetModuleHandleW(L"user32.dll");
        const auto getCtx = reinterpret_cast<HANDLE(WINAPI*)()>(GetProcAddress(user32, "GetThreadDpiAwarenessContext"));
        const auto getAwareness = reinterpret_cast<int(WINAPI*)(HANDLE)>(GetProcAddress(
            user32, "GetAwarenessFromDpiAwarenessContext"));
        const auto dpiForWindow = reinterpret_cast<UINT(WINAPI*)(HWND)>(GetProcAddress(user32, "GetDpiForWindow"));
        const auto dpiForSystem = reinterpret_cast<UINT(WINAPI*)()>(GetProcAddress(user32, "GetDpiForSystem"));

        const int awareness = (getCtx && getAwareness) ? getAwareness(getCtx()) : -1;
        const char* awarenessName = awareness == 0
                                        ? "unaware"
                                        : awareness == 1
                                        ? "system"
                                        : awareness == 2
                                        ? "per-monitor"
                                        : "unknown";

        UINT monDpiX = 0, monDpiY = 0;
        if (const HMODULE shcore = LoadLibraryA("shcore.dll"))
        {
            using GetDpiForMonitor_t = HRESULT(WINAPI*)(HMONITOR, int, UINT*, UINT*);
            if (const auto fn = reinterpret_cast<GetDpiForMonitor_t>(GetProcAddress(shcore, "GetDpiForMonitor")))
                fn(mon, 0 /* MDT_EFFECTIVE_DPI */, &monDpiX, &monDpiY);
            FreeLibrary(shcore);
        }

        int screenDcDpi = 0; // this is what UI::SetupUiLook derives the ImGui font size from
        if (HDC dc = GetDC(nullptr))
        {
            screenDcDpi = GetDeviceCaps(dc, LOGPIXELSX);
            ReleaseDC(nullptr, dc);
        }

        LogDebug("[WinDiag:%s] DPI awareness: %s, system DPI %u, window DPI %u, monitor DPI %u, screen DC DPI %d",
                 tag, awarenessName, dpiForSystem ? dpiForSystem() : 0, dpiForWindow ? dpiForWindow(hWnd) : 0, monDpiX,
                 screenDcDpi);
        LogDebug("[WinDiag:%s] GetSystemMetrics screen %dx%d", tag, GetSystemMetrics(SM_CXSCREEN),
                 GetSystemMetrics(SM_CYSCREEN));

        if (ImGui::GetCurrentContext())
        {
            const ImGuiIO& io = ImGui::GetIO();
            LogDebug("[WinDiag:%s] ImGui DisplaySize %.0fx%.0f, FontGlobalScale %.2f", tag, io.DisplaySize.x,
                     io.DisplaySize.y, io.FontGlobalScale);
        }
    }

    inline void ShutdownWindowHooks()
    {
        if (orig_ClipCursor)
        {
            orig_ClipCursor(nullptr);
        }
        if (g_hCBTHook)
        {
            UnhookWindowsHookEx(g_hCBTHook);
            g_hCBTHook = nullptr;
        }
        if (g_hGameWindow && g_origWndProc)
        {
            SetWindowLongPtrA(g_hGameWindow, GWLP_WNDPROC, (LONG_PTR)g_origWndProc);
            g_origWndProc = nullptr;
        }
    }

    // shared by CreateWindowExA/W: puts the main window on the configured monitor
    inline void AdjustMainWindow(DWORD& dwStyle, int& X, int& Y, int& nWidth, int& nHeight)
    {
        const Monitor::Target& mon = Monitor::Get();

        if (g_config.borderless)
        {
            dwStyle &= ~WS_OVERLAPPEDWINDOW;
            dwStyle |= WS_POPUP | WS_VISIBLE;
            X = mon.rect.left;
            Y = mon.rect.top;
            nWidth = mon.width(); // window always fills the monitor, backbuffer res is D3D's job
            nHeight = mon.height();
            Log("[Window] CreateWindowEx -> borderless %dx%d at (%d,%d)", nWidth, nHeight, X, Y);
            return;
        }

        if (g_config.resolutionW > 0 && g_config.resolutionH > 0)
        {
            nWidth = g_config.resolutionW;
            nHeight = g_config.resolutionH;
        }
        if (g_config.monitor > 0)
        {
            // plain windowed: create the window on the chosen monitor so its frame is built for that monitor's DPI
            // (the game creates it with size 0 and resizes it later, hook_SetWindowPos then centres it)
            if (nWidth > 0 && nHeight > 0)
            {
                X = mon.rect.left + (mon.width() - nWidth) / 2;
                Y = mon.rect.top + (mon.height() - nHeight) / 2;
            }
            else
            {
                X = mon.rect.left;
                Y = mon.rect.top;
            }
        }
        Log("[Window] CreateWindowEx -> windowed %dx%d at (%d,%d)", nWidth, nHeight, X, Y);
    }

    // grows a client rect to the window rect for a window with this style (DPI aware where the OS supports it)
    inline void AdjustRectForDpi(RECT& r, DWORD style, DWORD exStyle, UINT dpi)
    {
        using AdjustWindowRectExForDpi_t = BOOL(WINAPI*)(LPRECT, DWORD, BOOL, DWORD, UINT);
        static const auto fn = reinterpret_cast<AdjustWindowRectExForDpi_t>(
            GetProcAddress(GetModuleHandleW(L"user32.dll"), "AdjustWindowRectExForDpi"));
        if (fn)
            fn(&r, style, FALSE, exStyle, dpi);
        else
            AdjustWindowRectEx(&r, style, FALSE, exStyle);
    }

    inline UINT GetMonitorDpi(HMONITOR mon)
    {
        UINT dpiX = 0, dpiY = 0;
        if (mon)
        {
            HMODULE shcore = GetModuleHandleW(L"shcore.dll");
            if (!shcore)
                shcore = LoadLibraryA("shcore.dll");
            using GetDpiForMonitor_t = HRESULT(WINAPI*)(HMONITOR, int, UINT*, UINT*);
            if (shcore)
                if (const auto fn = reinterpret_cast<GetDpiForMonitor_t>(GetProcAddress(shcore, "GetDpiForMonitor")))
                    fn(mon, 0 /* MDT_EFFECTIVE_DPI */, &dpiX, &dpiY);
        }
        return dpiX ? dpiX : 96;
    }

    // "screen" the game sizes its render pipeline with, which also has to equal the client area ImGui and the overlays see:
    // borderless = the monitor, plain windowed = custom resolution or the largest client area whose window still fits the monitor
    inline SIZE GameScreenSize()
    {
        const Monitor::Target& mon = Monitor::Get();

        if (g_config.borderless)
            return {mon.width(), mon.height()};

        if (g_config.resolutionW > 0 && g_config.resolutionH > 0)
            return {g_config.resolutionW, g_config.resolutionH};

        RECT frame = {};
        AdjustRectForDpi(frame, WS_OVERLAPPEDWINDOW, 0, GetMonitorDpi(mon.handle));
        return {mon.width() - (frame.right - frame.left), mon.height() - (frame.bottom - frame.top)};
    }

    inline HWND WINAPI hook_CreateWindowExA(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle,
                                            int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu,
                                            HINSTANCE hInstance, LPVOID lpParam)
    {
        if (IsGameWindowClass(lpClassName) || IsMainGameWindow(hWndParent, dwStyle, nWidth, nHeight))
            AdjustMainWindow(dwStyle, X, Y, nWidth, nHeight);
        return orig_CreateWindowExA(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent,
                                    hMenu, hInstance, lpParam);
    }

    inline HWND WINAPI hook_CreateWindowExW(
        DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle,
        int X, int Y, int nWidth, int nHeight,
        HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
    {
        if (IsMainGameWindow(hWndParent, dwStyle, nWidth, nHeight))
            AdjustMainWindow(dwStyle, X, Y, nWidth, nHeight);
        return orig_CreateWindowExW(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent,
                                    hMenu, hInstance, lpParam);
    }

    inline BOOL WINAPI hook_SetWindowPos(
        HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags)
    {
        // only intercept the main (top-level, sizeable) window
        LONG style = GetWindowLongA(hWnd, GWL_STYLE);
        bool isTopLevel = (GetParent(hWnd) == nullptr);
        bool isSizable = (style & (WS_THICKFRAME | WS_POPUP)) != 0;

        if (isTopLevel && isSizable)
        {
            if (g_config.borderless)
            {
                // window must always fill the target monitor
                const Monitor::Target& mon = Monitor::Get();
                X = mon.rect.left;
                Y = mon.rect.top;
                cx = mon.width();
                cy = mon.height();
            }
            else if (!(uFlags & SWP_NOSIZE))
            {
                // plain windowed: the size the game asks for is the client area (= backbuffer, see hook_GetSystemMetrics),
                // so grow the window by its frame; a custom resolution takes precedence
                RECT r = {0, 0, cx, cy};
                if (g_config.resolutionW > 0 && g_config.resolutionH > 0)
                {
                    r.right = g_config.resolutionW;
                    r.bottom = g_config.resolutionH;
                }
                AdjustRectForDpi(r, static_cast<DWORD>(style), static_cast<DWORD>(GetWindowLongA(hWnd, GWL_EXSTYLE)),
                                 GetWindowDpi(hWnd));
                cx = r.right - r.left;
                cy = r.bottom - r.top;

                if (g_config.monitor > 0 && !(uFlags & SWP_NOMOVE))
                {
                    // keep the window centred on the chosen monitor instead of wherever the game puts it
                    const Monitor::Target& mon = Monitor::Get();
                    X = mon.rect.left + (mon.width() - cx) / 2;
                    Y = mon.rect.top + (mon.height() - cy) / 2;
                }
            }
        }
        return orig_SetWindowPos(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
    }

    inline LONG WINAPI hook_ChangeDisplaySettingsA(DEVMODEA* lpDevMode, DWORD dwFlags)
    {
        LogDebug("[Window] Suppressed ChangeDisplaySettingsA");
        return DISP_CHANGE_SUCCESSFUL;
    }

    inline LONG WINAPI hook_ChangeDisplaySettingsW(DEVMODEW* lpDevMode, DWORD dwFlags)
    {
        LogDebug("[Window] Suppressed ChangeDisplaySettingsW");
        return DISP_CHANGE_SUCCESSFUL;
    }

    inline LONG WINAPI hook_SetWindowLongA(HWND hWnd, int nIndex, LONG dwNewLong)
    {
        if (!g_config.borderless
            && hWnd == g_hGameWindow && nIndex == GWL_STYLE)
        {
            dwNewLong |= WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
            dwNewLong &= ~WS_POPUP;
            LogDebug("[Window] SetWindowLongA(GWL_STYLE) - preserved caption");
        }
        return orig_SetWindowLongA(hWnd, nIndex, dwNewLong);
    }

    inline LONG WINAPI hook_SetWindowLongW(HWND hWnd, int nIndex, LONG dwNewLong)
    {
        if (!g_config.borderless
            && hWnd == g_hGameWindow && nIndex == GWL_STYLE)
        {
            dwNewLong |= WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
            dwNewLong &= ~WS_POPUP;
            LogDebug("[Window] SetWindowLongW(GWL_STYLE) - preserved caption");
        }
        return orig_SetWindowLongW(hWnd, nIndex, dwNewLong);
    }

    // game takes its "screen resolution" from SM_CXSCREEN / SM_CYSCREEN (primary monitor) and sizes its whole render pipeline with it.
    // Report GameScreenSize() instead so game, window, backbuffer, ImGui and overlays all agree.
    // Identical to the real value when borderless on the primary monitor.
    inline int WINAPI hook_GetSystemMetrics(int nIndex)
    {
        if (nIndex == SM_CXSCREEN || nIndex == SM_CYSCREEN)
        {
            if (Monitor::Get().handle)
            // null while the monitor is being resolved or if enumeration failed -> real value
            {
                static const SIZE s_size = GameScreenSize(); // constant for the whole run
                return nIndex == SM_CXSCREEN ? s_size.cx : s_size.cy;
            }
        }
        return orig_GetSystemMetrics(nIndex);
    }

    // -------------------------------------------------------------------------
    // Input Hooks
    // -------------------------------------------------------------------------
    inline SHORT WINAPI hook_GetKeyState(int nVirtKey)
    {
        if (UI::Get()->IsVisible())
            return 0;
        return orig_GetKeyState(nVirtKey);
    }

    inline BOOL WINAPI hook_GetCursorPos(LPPOINT lpPoint)
    {
        BOOL result = orig_GetCursorPos(lpPoint);
        if (UI::Get()->IsVisible())
        {
            if (lpPoint) *lpPoint = g_frozenCursorPos;
        }
        else if (result && lpPoint)
        {
            g_frozenCursorPos = *lpPoint;
        }
        return result;
    }

    inline BOOL WINAPI hook_SetCursorPos(int X, int Y)
    {
        if (UI::Get()->IsVisible())
            return TRUE;
        return orig_SetCursorPos(X, Y);
    }


    // -------------------------------------------------------------------------
    // Init
    // -------------------------------------------------------------------------
    inline bool InitWindowHooks()
    {
        auto mh = [](const char* name, auto hook, auto* orig, const wchar_t* mod = L"user32.dll")
        {
            MH_STATUS s = MH_CreateHookApiEx(mod, name, (void*)hook, (void**)orig, nullptr);
            if (s != MH_OK)
            {
                Log("[Window] Failed to hook %s: %d", name, s);
                return false;
            }
            return true;
        };

        bool ok = true;
        ok &= mh("CreateWindowExA",         &hook_CreateWindowExA,          &orig_CreateWindowExA);
        ok &= mh("SetWindowPos",            &hook_SetWindowPos,             &orig_SetWindowPos);
        ok &= mh("ChangeDisplaySettingsA",  &hook_ChangeDisplaySettingsA,   &orig_ChangeDisplaySettingsA);
        ok &= mh("CreateWindowExW",         &hook_CreateWindowExW,          &orig_CreateWindowExW);
        ok &= mh("ChangeDisplaySettingsW",  &hook_ChangeDisplaySettingsW,   &orig_ChangeDisplaySettingsW);
        ok &= mh("ShowWindow",              &hook_ShowWindow,               &orig_ShowWindow);
        ok &= mh("ClipCursor",              &hook_ClipCursor,               &orig_ClipCursor);
        ok &= mh("ShowCursor",              &hook_ShowCursor,               &orig_ShowCursor);
        ok &= mh("SetWindowLongA",          &hook_SetWindowLongA,           &orig_SetWindowLongA);
        ok &= mh("SetWindowLongW",          &hook_SetWindowLongW,           &orig_SetWindowLongW);
        ok &= mh("GetKeyState",             &hook_GetKeyState,              &orig_GetKeyState);
        ok &= mh("GetCursorPos",            &hook_GetCursorPos,             &orig_GetCursorPos);
        ok &= mh("SetCursorPos",            &hook_SetCursorPos,             &orig_SetCursorPos);
        ok &= mh("GetSystemMetrics",        &hook_GetSystemMetrics,         &orig_GetSystemMetrics);

        if (ok) Log("[Window] Hooks installed");
        return ok;
    }
}