#pragma once
#include "log.h"
#include <Windows.h>
#include <hidsdi.h>
#include <MinHook.h>
#include "portal_device.h"

namespace ssa::KernelHooks
{
    // ---------------------------------------------------------------------------
    // Helpers
    // ---------------------------------------------------------------------------

    inline void WideToLog(const wchar_t* wide, char* out, size_t out_len)
    {
        if (!wide) { strncpy_s(out, out_len, "(null)", out_len); return; }
        WideCharToMultiByte(CP_UTF8, 0, wide, -1, out, (int)out_len, nullptr, nullptr);
    }

    inline bool LooksLikeRealPortal(const wchar_t* path)
    {
        if (!path) return false;
        wchar_t lower[512] = {};
        wcsncpy_s(lower, path, 511);
        _wcslwr_s(lower);
        return wcsstr(lower, L"vid_1430") && wcsstr(lower, L"pid_0150");
    }

    inline void LogHex(const char* label, const BYTE* buf, size_t len)
    {
        char line[256] = {};
        int offset = 0;
        for (size_t i = 0; i < len; i++) {
            offset += sprintf_s(line + offset, sizeof(line) - offset,
                "%02X%s", buf[i], ((i + 1) % 8 == 0 && i + 1 < len) ? " | " : " ");
        }
        Log("%s [%zu bytes]: %s", label, len, line);
    }

    // real (HID driver) portal handle tracking - still needed for pass-through
    // logging when the HID driver is installed.
    inline HANDLE g_portal_handles[8] = {
        INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE,
        INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE
    };
    inline void TrackPortalHandle(HANDLE h)   { for (auto& s : g_portal_handles) if (s == INVALID_HANDLE_VALUE) { s = h; return; } }
    inline void UntrackPortalHandle(HANDLE h) { for (auto& s : g_portal_handles) if (s == h) { s = INVALID_HANDLE_VALUE; return; } }
    inline bool IsRealPortalHandle(HANDLE h)  { for (auto  s : g_portal_handles) if (s == h) return true; return false; }

    // buffer / overlapped saved from the last ReadFile on a real portal handle
    inline LPVOID g_pending_real_buf   = nullptr;
    inline bool   g_overlapped_pattern_logged = false;
    static constexpr int IN_LOG_LIMIT = 40;
    inline int    g_in_log_count = 0;

    inline HANDLE g_portal_overlapped_event = nullptr;

    // ---------------------------------------------------------------------------
    // CreateFileW
    // ---------------------------------------------------------------------------

    using CreateFileW_t = HANDLE(WINAPI*)(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
    inline CreateFileW_t orig_CreateFileW = nullptr;

    inline HANDLE WINAPI hook_CreateFileW(
        LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
        LPSECURITY_ATTRIBUTES lpSA, DWORD dwCreation,
        DWORD dwFlags, HANDLE hTemplate)
    {
        // ---- FAKE PATH (WinUSB mode) ----
        if (Portal::IsFakePath(lpFileName)) {
            Portal::HKind kind;
            if (dwDesiredAccess == 0) {
                kind = Portal::HKind::PROBE;
            } else if (dwFlags & FILE_FLAG_OVERLAPPED) {
                kind = Portal::HKind::REAL;
            } else {
                kind = Portal::HKind::CAPABILITY;
            }

            HANDLE h = Portal::AllocFake(kind);
            if (h == INVALID_HANDLE_VALUE) return INVALID_HANDLE_VALUE;

            const char* kname = kind == Portal::HKind::PROBE       ? "PROBE"
                               : kind == Portal::HKind::CAPABILITY ? "CAPABILITY"
                               :                                      "REAL (overlapped)";
            LogDebug("[Portal] CreateFileW(FAKE %s) => handle=%p", kname, h);

            if (kind == Portal::HKind::REAL) {
                if (!Portal::OpenUSB()) {
                    Portal::FreeFake(h);
                    SetLastError(ERROR_DEVICE_NOT_CONNECTED);
                    return INVALID_HANDLE_VALUE;
                }
            }
            return h;
        }

        // ---- REAL HID PATH (pass-through + logging) ----
        HANDLE result = orig_CreateFileW(lpFileName, dwDesiredAccess, dwShareMode,
            lpSA, dwCreation, dwFlags, hTemplate);

        if (LooksLikeRealPortal(lpFileName)) {
            const char* kind = (dwDesiredAccess == 0) ? "PROBE"
                             : (dwFlags & FILE_FLAG_OVERLAPPED) ? "REAL (overlapped)"
                             : "CAPABILITY SCAN (no overlapped)";
            LogDebug("[Portal] CreateFileW(PORTAL %s) => handle=%p", kind, result);
            LogDebug("             access=0x%08X  share=0x%08X  flags=0x%08X", dwDesiredAccess, dwShareMode, dwFlags);
            if (result != INVALID_HANDLE_VALUE) TrackPortalHandle(result);
        }
        return result;
    }

    // ---------------------------------------------------------------------------
    // ReadFile
    // ---------------------------------------------------------------------------

    using ReadFile_t = BOOL(WINAPI*)(HANDLE, LPVOID, DWORD, LPDWORD, LPOVERLAPPED);
    inline ReadFile_t orig_ReadFile = nullptr;

    inline BOOL WINAPI hook_ReadFile(
        HANDLE hFile, LPVOID lpBuffer, DWORD nBytesToRead,
        LPDWORD lpBytesRead, LPOVERLAPPED lpOverlapped)
    {
        // ---- FAKE REAL HANDLE ----
        if (Portal::GetKind(hFile) == Portal::HKind::REAL) {
            // arm the reader thread -> it will fill lpBuffer and signal ovlp->hEvent
            Portal::ArmRead(static_cast<BYTE*>(lpBuffer), lpOverlapped, hFile);
            // return as if the I/O is pending
            SetLastError(ERROR_IO_PENDING);
            return FALSE;
        }

        // ---- REAL HID HANDLE ----
        BOOL result = orig_ReadFile(hFile, lpBuffer, nBytesToRead, lpBytesRead, lpOverlapped);

        if (IsRealPortalHandle(hFile) && lpOverlapped) {
            g_pending_real_buf = lpBuffer;

            if (!g_overlapped_pattern_logged) {
                g_portal_overlapped_event = lpOverlapped->hEvent;
                if (lpOverlapped->hEvent && lpOverlapped->hEvent != INVALID_HANDLE_VALUE)
                    Log("OVERLAPPED PATTERN: B - hEvent=%p", lpOverlapped->hEvent);
                else
                    Log("OVERLAPPED PATTERN: A - no event");
                g_overlapped_pattern_logged = true;
            }
        }
        return result;
    }

    // ---------------------------------------------------------------------------
    // GetOverlappedResult
    // Note: For fake handles, GetOverlappedResult works without interception
    // because our reader thread sets ovlp->Internal and ovlp->InternalHigh
    // directly before signalling hEvent. We hook it only for pass-through
    // logging on real handles.
    // ---------------------------------------------------------------------------

    using GetOverlappedResult_t = BOOL(WINAPI*)(HANDLE, LPOVERLAPPED, LPDWORD, BOOL);
    inline GetOverlappedResult_t orig_GetOverlappedResult = nullptr;

    inline BOOL WINAPI hook_GetOverlappedResult(
        HANDLE hFile, LPOVERLAPPED lpOverlapped, LPDWORD lpBytesTransferred, BOOL bWait)
    {
        // fake handles: pass through (OVERLAPPED fields already set by reader thread)
        if (Portal::IsFake(hFile))
            return orig_GetOverlappedResult(hFile, lpOverlapped, lpBytesTransferred, bWait);

        // real portal handle: pass through + log
        BOOL result = orig_GetOverlappedResult(hFile, lpOverlapped, lpBytesTransferred, bWait);

        if (!IsRealPortalHandle(hFile) || !result) return result;

        DWORD bytes = lpBytesTransferred ? *lpBytesTransferred : 0;
        if (g_pending_real_buf && bytes > 0) {
            if (g_in_log_count < IN_LOG_LIMIT && g_config.logLevel >= LogLevel::DEBUG) {
                LogHex("IN", static_cast<BYTE*>(g_pending_real_buf), bytes);
                if (++g_in_log_count == IN_LOG_LIMIT)
                    Log("IN data limit reached (%d), suppressing", IN_LOG_LIMIT);
            }
            g_pending_real_buf = nullptr;
        } else if (bytes == 0) {
            static bool once = false;
            if (!once) { Log("GetOverlappedResult(portal) => bytes=0 (drain)"); once = true; }
        }
        return result;
    }

    // ---------------------------------------------------------------------------
    // WaitForSingleObject - filter to portal event only
    // ---------------------------------------------------------------------------

    using WaitForSingleObject_t = DWORD(WINAPI*)(HANDLE, DWORD);
    inline WaitForSingleObject_t orig_WaitForSingleObject = nullptr;

    inline DWORD WINAPI hook_WaitForSingleObject(HANDLE hHandle, DWORD dwMs)
    {
        DWORD result = orig_WaitForSingleObject(hHandle, dwMs);

        if (g_portal_overlapped_event && hHandle == g_portal_overlapped_event) {
            const char* r = result == WAIT_OBJECT_0  ? "SIGNALED"
                          : result == WAIT_TIMEOUT   ? "TIMEOUT"
                          : "OTHER";
            LogDebug("WaitForSingleObject(PORTAL EVENT, timeout=%lu) => %s", dwMs, r);
        }
        return result;
    }

    // ---------------------------------------------------------------------------
    // CloseHandle
    // ---------------------------------------------------------------------------

    using CloseHandle_t = BOOL(WINAPI*)(HANDLE);
    inline CloseHandle_t orig_CloseHandle = nullptr;

    inline BOOL WINAPI hook_CloseHandle(HANDLE hObject)
    {
        // ---- FAKE HANDLE ----
        Portal::HKind kind = Portal::GetKind(hObject);
        if (kind != Portal::HKind::NONE) {
            Log("[Portal] CloseHandle(fake %s handle=%p)",
                kind == Portal::HKind::PROBE       ? "PROBE"
              : kind == Portal::HKind::CAPABILITY  ? "CAPABILITY"
              :                                      "REAL", hObject);

            if (kind == Portal::HKind::REAL)
            {
                // only clear read state if this handle actually owns the current read
                if (Portal::g_io) {
                    std::lock_guard<std::mutex> lk(Portal::g_io->mtx);
                    if (Portal::g_io->owner == hObject) {
                        Portal::g_io->buf   = nullptr;
                        Portal::g_io->ovlp  = nullptr;
                        Portal::g_io->arm   = false;
                        Portal::g_io->owner = nullptr;
                        Log("[Portal] CloseHandle cleared read state (was owner)");
                    }
                }
                Portal::CloseUSB();
            }

            Portal::FreeFake(hObject); // also calls real CloseHandle on the event
            return TRUE;
        }

        // ---- REAL PORTAL HANDLE ----
        if (IsRealPortalHandle(hObject)) {
            Log("[Portal] CloseHandle(portal=%p)", hObject);
            UntrackPortalHandle(hObject);
        }
        return orig_CloseHandle(hObject);
    }

    // ---------------------------------------------------------------------------
    // CancelIo / CancelIoEx - not called by portal.dll, kept for safety
    // ---------------------------------------------------------------------------

    using CancelIo_t   = BOOL(WINAPI*)(HANDLE);
    using CancelIoEx_t = BOOL(WINAPI*)(HANDLE, LPOVERLAPPED);
    inline CancelIo_t   orig_CancelIo   = nullptr;
    inline CancelIoEx_t orig_CancelIoEx = nullptr;

    inline BOOL WINAPI hook_CancelIo(HANDLE hFile) {
        if (Portal::IsFake(hFile)) { Log("[Portal] CancelIo(fake)"); return TRUE; }
        return orig_CancelIo(hFile);
    }
    inline BOOL WINAPI hook_CancelIoEx(HANDLE hFile, LPOVERLAPPED ovlp) {
        if (Portal::IsFake(hFile)) { Log("[Portal] CancelIoEx(fake)"); return TRUE; }
        return orig_CancelIoEx(hFile, ovlp);
    }

    // ---------------------------------------------------------------------------
    // Hook installation
    // ---------------------------------------------------------------------------
    inline bool InitKernelHooks()
    {
        auto mh = [](const char* name, auto hook, auto* orig, const wchar_t* mod = L"kernel32") {
            MH_STATUS s = MH_CreateHookApiEx(mod, name, hook, (void**)orig, nullptr);
            if (s != MH_OK) { Log("[Kernel] Failed to hook %s: %d", name, s); return false; }
            return true;
        };

        if (!mh("CreateFileW",         &hook_CreateFileW,         &orig_CreateFileW))         return false;
        if (!mh("ReadFile",            &hook_ReadFile,            &orig_ReadFile))            return false;
        if (!mh("GetOverlappedResult", &hook_GetOverlappedResult, &orig_GetOverlappedResult)) return false;
        if (!mh("CloseHandle",         &hook_CloseHandle,         &orig_CloseHandle))         return false;
        if (!mh("WaitForSingleObject", &hook_WaitForSingleObject, &orig_WaitForSingleObject)) return false;
        if (!mh("CancelIo",            &hook_CancelIo,            &orig_CancelIo))            return false;
        if (!mh("CancelIoEx",          &hook_CancelIoEx,          &orig_CancelIoEx))          return false;

        Log("[Kernel] Kernel hooks installed");
        return true;
    }
} // namespace ssa::KernelHooks