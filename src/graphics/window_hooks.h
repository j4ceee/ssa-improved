#pragma once
#include <Windows.h>
#include "config.h"
#include "log.h"
#include "MinHook.h"

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

    inline WNDPROC                  g_origWndProc               = nullptr;
    inline HHOOK                    g_hCBTHook                  = nullptr;

    inline bool g_hasFocus      = true;
    inline RECT g_gameClipRect  = {};
    inline bool g_hasClipRect   = false;

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

    inline BOOL WINAPI hook_ClipCursor(const RECT* pRect)
    {
        if (pRect) {
            // game is setting a clip rect - save it but only apply if we have focus
            g_gameClipRect = *pRect;
            g_hasClipRect = true;
            if (!g_hasFocus) {
                LogDebug("[Window] Deferred ClipCursor (no focus)");
                return TRUE;
            }
        } else {
            // game is releasing the clip - honour it and clear ours
            g_hasClipRect = false;
        }
        return orig_ClipCursor(pRect);
    }

    inline int WINAPI hook_ShowCursor(BOOL bShow)
    {
        // if we don't have focus, always show the cursor regardless of what the game wants
        if (g_config.windowed && !g_hasFocus && !bShow) {
            LogDebug("[Window] Suppressed ShowCursor(FALSE) - no focus");
            return 0; // return a plausible cursor count without actually hiding
        }
        return orig_ShowCursor(bShow);
    }

    inline BOOL WINAPI hook_ShowWindow(HWND hWnd, int nCmdShow)
    {
        if (g_config.windowed && g_config.borderless && hWnd == g_hGameWindow && nCmdShow == SW_MINIMIZE) {
            LogDebug("[Window] Suppressed ShowWindow(SW_MINIMIZE)");
            return TRUE;
        }
        return orig_ShowWindow(hWnd, nCmdShow);
    }

    inline LRESULT CALLBACK GameWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        if (g_config.windowed) {
            switch (uMsg) {
                case WM_ACTIVATE:
                    if (LOWORD(wParam) == WA_INACTIVE) {
                        g_hasFocus = false;
                        orig_ClipCursor(nullptr); // release mouse to whole desktop
                        orig_ShowCursor(TRUE); // ensure cursor is visible when alt-tabbed
                        Log("[Window] Suppressed WM_ACTIVATE(WA_INACTIVE), released ClipCursor");
                        return 0;
                    } else {
                        g_hasFocus = true;
                        if (g_hasClipRect)
                            orig_ClipCursor(&g_gameClipRect); // restore game's clip rect
                        orig_ShowCursor(FALSE);
                        LogDebug("[Window] WM_ACTIVATE focus gained, restored ClipCursor");
                    }
                    break;

                case WM_ACTIVATEAPP:
                    if (!wParam) {
                        // already handled by WM_ACTIVATE above, just suppress
                        Log("[Window] Suppressed WM_ACTIVATEAPP focus-loss");
                        return 0;
                    }
                    break;

                case WM_SIZE:
                    // windows can minimize us at the OS level bypassing WndProc entirely, catch resulting WM_SIZE & restore
                    if (wParam == SIZE_MINIMIZED) {
                        Log("[Window] Caught SIZE_MINIMIZED - restoring");
                        orig_ShowWindow(hWnd, SW_RESTORE);
                        return 0;
                    }
                    break;

                case WM_SYSCOMMAND:
                    if ((wParam & 0xFFF0) == SC_MINIMIZE) {
                        Log("[Window] Suppressed SC_MINIMIZE");
                        return 0;
                    }
                    break;

                default:
                    break;
            }
        }
        return CallWindowProcA(g_origWndProc, hWnd, uMsg, wParam, lParam);
    }

    inline LRESULT CALLBACK CBTProc(int nCode, WPARAM wParam, LPARAM lParam)
    {
        if (nCode == HCBT_MINMAX && g_config.windowed) {
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
        struct FindCtx { HWND result; DWORD pid; };
        FindCtx ctx = { nullptr, GetCurrentProcessId() };

        EnumWindows([](HWND hWnd, LPARAM lParam) -> BOOL {
            auto* ctx = reinterpret_cast<FindCtx*>(lParam);
            DWORD pid = 0;
            GetWindowThreadProcessId(hWnd, &pid);
            if (pid != ctx->pid) return TRUE;

            LONG style = GetWindowLongA(hWnd, GWL_STYLE);
            RECT r = {};
            GetWindowRect(hWnd, &r);
            int w = r.right  - r.left;
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

    inline void ShutdownWindowHooks()
    {
        if (orig_ClipCursor) {
            orig_ClipCursor(nullptr);
        }
        if (g_hCBTHook) {
            UnhookWindowsHookEx(g_hCBTHook);
            g_hCBTHook = nullptr;
        }
        if (g_hGameWindow && g_origWndProc) {
            SetWindowLongPtrA(g_hGameWindow, GWLP_WNDPROC, (LONG_PTR)g_origWndProc);
            g_origWndProc = nullptr;
        }
    }

    inline HWND WINAPI hook_CreateWindowExA(
        DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle,
        int X, int Y, int nWidth, int nHeight,
        HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
    {
        if (g_config.windowed && IsMainGameWindow(hWndParent, dwStyle, nWidth, nHeight)) {
            if (g_config.borderless) {
                dwStyle &= ~WS_OVERLAPPEDWINDOW;
                dwStyle |= WS_POPUP | WS_VISIBLE;
                X = 0; Y = 0;
                nWidth  = GetSystemMetrics(SM_CXSCREEN);  // window always desktop-sized
                nHeight = GetSystemMetrics(SM_CYSCREEN);  // backbuffer res is D3D's job
                Log("[Window] CreateWindowExA → borderless %dx%d", nWidth, nHeight);
            } else if (g_config.resolutionW > 0 && g_config.resolutionH > 0) {
                nWidth  = g_config.resolutionW;
                nHeight = g_config.resolutionH;
                Log("[Window] CreateWindowExA → windowed %dx%d", nWidth, nHeight);
            }
        }
        return orig_CreateWindowExA(dwExStyle, lpClassName, lpWindowName, dwStyle,
            X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
    }

    inline HWND WINAPI hook_CreateWindowExW(
        DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle,
        int X, int Y, int nWidth, int nHeight,
        HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
    {
        if (g_config.windowed && IsMainGameWindow(hWndParent, dwStyle, nWidth, nHeight)) {
            if (g_config.borderless) {
                dwStyle &= ~WS_OVERLAPPEDWINDOW;
                dwStyle |= WS_POPUP | WS_VISIBLE;
                X = 0; Y = 0;
                nWidth  = GetSystemMetrics(SM_CXSCREEN);  // window always desktop-sized
                nHeight = GetSystemMetrics(SM_CYSCREEN);  // backbuffer res is D3D's job
                Log("[Window] CreateWindowExA → borderless %dx%d", nWidth, nHeight);
            } else if (g_config.resolutionW > 0 && g_config.resolutionH > 0) {
                nWidth  = g_config.resolutionW;
                nHeight = g_config.resolutionH;
                Log("[Window] CreateWindowExA → windowed %dx%d", nWidth, nHeight);
            }
        }
        return orig_CreateWindowExW(dwExStyle, lpClassName, lpWindowName, dwStyle,
            X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
    }

    inline BOOL WINAPI hook_SetWindowPos(
        HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags)
    {
        if (g_config.windowed) {
            // only intercept the main (top-level, sizeable) window
            LONG style = GetWindowLongA(hWnd, GWL_STYLE);
            bool isTopLevel = (GetParent(hWnd) == nullptr);
            bool isSizable  = (style & (WS_THICKFRAME | WS_POPUP)) != 0;

            if (isTopLevel && isSizable) {
                if (g_config.borderless) {
                    // window must always fill the desktop
                    X = 0; Y = 0;
                    cx = GetSystemMetrics(SM_CXSCREEN);
                    cy = GetSystemMetrics(SM_CYSCREEN);
                } else if (g_config.resolutionW > 0 && g_config.resolutionH > 0) {
                    // stop game from overriding our custom windowed size
                    cx = g_config.resolutionW;
                    cy = g_config.resolutionH;
                }
            }
        }
        return orig_SetWindowPos(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
    }

    inline LONG WINAPI hook_ChangeDisplaySettingsA(DEVMODEA* lpDevMode, DWORD dwFlags)
    {
        if (!g_config.windowed) return orig_ChangeDisplaySettingsA(lpDevMode, dwFlags);
        LogDebug("[Window] Suppressed ChangeDisplaySettingsA");
        return DISP_CHANGE_SUCCESSFUL;
    }

    inline LONG WINAPI hook_ChangeDisplaySettingsW(DEVMODEW* lpDevMode, DWORD dwFlags)
    {
        if (!g_config.windowed) return orig_ChangeDisplaySettingsW(lpDevMode, dwFlags);
        LogDebug("[Window] Suppressed ChangeDisplaySettingsW");
        return DISP_CHANGE_SUCCESSFUL;
    }

    inline LONG WINAPI hook_SetWindowLongA(HWND hWnd, int nIndex, LONG dwNewLong)
    {
        if (g_config.windowed && !g_config.borderless
            && hWnd == g_hGameWindow && nIndex == GWL_STYLE)
        {
            dwNewLong |=  WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
            dwNewLong &= ~WS_POPUP;
            LogDebug("[Window] SetWindowLongA(GWL_STYLE) - preserved caption");
        }
        return orig_SetWindowLongA(hWnd, nIndex, dwNewLong);
    }

    inline LONG WINAPI hook_SetWindowLongW(HWND hWnd, int nIndex, LONG dwNewLong)
    {
        if (g_config.windowed && !g_config.borderless
            && hWnd == g_hGameWindow && nIndex == GWL_STYLE)
        {
            dwNewLong |=  WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
            dwNewLong &= ~WS_POPUP;
            LogDebug("[Window] SetWindowLongW(GWL_STYLE) - preserved caption");
        }
        return orig_SetWindowLongW(hWnd, nIndex, dwNewLong);
    }

    inline bool InitWindowHooks()
    {
        auto mh = [](const char* name, auto hook, auto* orig, const wchar_t* mod = L"user32.dll") {
            MH_STATUS s = MH_CreateHookApiEx(mod, name, (void*)hook, (void**)orig, nullptr);
            if (s != MH_OK) { Log("[Window] Failed to hook %s: %d", name, s); return false; }
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

        if (ok) Log("[Window] Hooks installed");
        return ok;
    }
}