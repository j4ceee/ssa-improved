#include "log.h"
#include <Windows.h>
#include <hidsdi.h>
#include <MinHook.h>

namespace ssa::KernelHooks
{
    // ---------------------------------------------------------------------------
    // Helpers
    // ---------------------------------------------------------------------------

    // convert wide string to narrow for logging
    inline void WideToLog(const wchar_t* wide, char* out, size_t out_len)
    {
        if (!wide) { strncpy_s(out, out_len, "(null)", out_len); return; }
        WideCharToMultiByte(CP_UTF8, 0, wide, -1, out, (int)out_len, nullptr, nullptr);
    }

    // is this handle/path related to the portal?
    inline bool LooksLikePortal(const wchar_t* path)
    {
        if (!path) return false;
        // HID path will contain vid_1430 and pid_0150
        return (wcsstr(path, L"1430") != nullptr || wcsstr(path, L"0150") != nullptr
             || wcsstr(path, L"hid") != nullptr  || wcsstr(path, L"HID") != nullptr);
    }

    // ---------------------------------------------------------------------------
    // Hooked functions
    // ---------------------------------------------------------------------------

    // -- CreateFileW --

    using CreateFileW_t = HANDLE(WINAPI*)(
        LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
        LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
        DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);

    inline CreateFileW_t orig_CreateFileW = nullptr;

    inline HANDLE WINAPI hook_CreateFileW(
        LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
        LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
        DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
    {
        char path_narrow[512] = {};
        WideToLog(lpFileName, path_narrow, sizeof(path_narrow));

        HANDLE result = orig_CreateFileW(lpFileName, dwDesiredAccess, dwShareMode,
            lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);

        // Log all calls so we catch the exact path portal.dll uses
        // Log("CreateFileW(\"%s\") => handle=%p%s",
        //     path_narrow, result,
        //     LooksLikePortal(lpFileName) ? "  << PORTAL PATH" : "");

        return result;
    }

    // -- ReadFile --

    using ReadFile_t = BOOL(WINAPI*)(
    HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead,
    LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped);

    inline ReadFile_t orig_ReadFile = nullptr;

    inline BOOL WINAPI hook_ReadFile(
        HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead,
        LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped)
    {
        BOOL result = orig_ReadFile(hFile, lpBuffer, nNumberOfBytesToRead,
            lpNumberOfBytesRead, lpOverlapped);

        // Log("ReadFile(handle=%p, size=%lu, overlapped=%s) => %s bytes=%lu firstbyte=0x%02X",
        //     hFile,
        //     nNumberOfBytesToRead,
        //     lpOverlapped ? "YES" : "NO (sync)",   // ← answers the overlapped question
        //     result ? "OK" : "FAIL",
        //     lpNumberOfBytesRead ? *lpNumberOfBytesRead : 0,
        //     (result && lpBuffer && lpNumberOfBytesRead && *lpNumberOfBytesRead > 0)
        //         ? ((BYTE*)lpBuffer)[0] : 0);

        return result;
    }

    // -- HidD_SetOutputReport (hid.dll) --

    using HidD_SetOutputReport_t = BOOLEAN(WINAPI*)(
    HANDLE HidDeviceObject, PVOID ReportBuffer, ULONG ReportBufferLength);

    inline HidD_SetOutputReport_t orig_HidD_SetOutputReport = nullptr;

    inline BOOLEAN WINAPI hook_HidD_SetOutputReport(
        HANDLE HidDeviceObject, PVOID ReportBuffer, ULONG ReportBufferLength)
    {
        char hex[64] = {};
        BYTE* buf = (BYTE*)ReportBuffer;
        int log_bytes = std::min((ULONG)8, ReportBufferLength);
        int offset = 0;
        for (int i = 0; i < log_bytes; i++)
            offset += sprintf_s(hex + offset, sizeof(hex) - offset, "%02X ", buf[i]);

        // Log("HidD_SetOutputReport(handle=%p, len=%lu) data=[%s...]",
        //     HidDeviceObject, ReportBufferLength, hex);

        return orig_HidD_SetOutputReport(HidDeviceObject, ReportBuffer, ReportBufferLength);
    }

    // -- CloseHandle --

    using CloseHandle_t = BOOL(WINAPI*)(HANDLE hObject);

    inline CloseHandle_t orig_CloseHandle = nullptr;

    inline BOOL WINAPI hook_CloseHandle(HANDLE hObject)
    {
        BOOL result = orig_CloseHandle(hObject);
        // Log("CloseHandle(handle=%p) => %d", hObject, result);
        return result;
    }

    // ---------------------------------------------------------------------------
    // Hook installation
    // ---------------------------------------------------------------------------

    inline bool InitKernelHooks()
    {
        if (MH_CreateHookApiEx(L"kernel32", "CreateFileW",
        &hook_CreateFileW, (void**)&orig_CreateFileW, nullptr) != MH_OK) {
            Log("Failed to hook CreateFileW");
            return false;
        }

        if (MH_CreateHookApiEx(L"kernel32", "ReadFile",
            &hook_ReadFile, (void**)&orig_ReadFile, nullptr) != MH_OK)
        {
            Log("Failed to hook ReadFile");
            return false;
        }

        MH_STATUS hid_status = MH_CreateHookApiEx(L"hid", "HidD_SetOutputReport",
            &hook_HidD_SetOutputReport, (void**)&orig_HidD_SetOutputReport, nullptr);
        if (hid_status != MH_OK)
        {
            Log("HidD_SetOutputReport hook deferred (hid.dll not yet loaded): status=%d", hid_status);
            // Non-fatal for the logging phase — CreateFileW alone answers most questions
        }

        if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) {
            Log("Enabling Kernel hooks failed");
            return false;
        }

        return true;
    }

}