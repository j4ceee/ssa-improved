#pragma once
#include <Windows.h>
#include <setupapi.h>
#include "log.h"
#include "MinHook.h"
#include "portal_device.h"

// ---------------------------------------------------------------------------
// SetupAPI injection
// When WinUSB driver is installed, the portal doesn't appear in the HID
// device class enumeration. We inject a fake entry so portal.dll finds it.
//
// Injection mechanism:
//   1. SetupDiEnumDeviceInterfaces: when the scan exhausts (NO_MORE_ITEMS)
//      and the GUID is the HID interface GUID, we return a fake entry.
//      The fake entry is marked with a sentinel value in Reserved.
//   2. SetupDiGetDeviceInterfaceDetailW: when called with our sentinel,
//      we return our fake device path instead of calling the real API.
//   3. CreateFileW in kernel.h recognises the fake path and routes to libusb.
// ---------------------------------------------------------------------------
namespace ssa::SetupApiHooks
{
    // HID interface class GUID: {4D1E55B2-F16F-11CF-88CB-001111000030}
    inline bool GuidIsHid(const GUID* g)
    {
        if (!g) return false;
        return g->Data1 == 0x4D1E55B2 && g->Data2 == 0xF16F && g->Data3 == 0x11CF
            && g->Data4[0] == 0x88 && g->Data4[1] == 0xCB
            && g->Data4[2] == 0x00 && g->Data4[3] == 0x11
            && g->Data4[4] == 0x11 && g->Data4[5] == 0x00
            && g->Data4[6] == 0x00 && g->Data4[7] == 0x30;
    }

    // per-scan injection state
    // - reset at the start of each new scan (MemberIndex == 0)
    // - ensures we inject exactly once per scan, not once per session
    inline bool g_injected_this_scan = false;

    // ---------------------------------------------------------------------------
    // SetupDiEnumDeviceInterfaces
    // ---------------------------------------------------------------------------

    using SetupDiEnumDeviceInterfaces_t = BOOL(WINAPI*)(HDEVINFO, PSP_DEVINFO_DATA, const GUID*, DWORD, PSP_DEVICE_INTERFACE_DATA);
    inline SetupDiEnumDeviceInterfaces_t orig_SetupDiEnumDeviceInterfaces = nullptr;

    inline BOOL WINAPI hook_SetupDiEnumDeviceInterfaces(
        HDEVINFO DeviceInfoSet, PSP_DEVINFO_DATA DeviceInfoData,
        const GUID* InterfaceClassGuid, DWORD MemberIndex,
        PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData)
    {
        // reset injection state at the start of each scan
        if (MemberIndex == 0)
            g_injected_this_scan = false;

        BOOL result = orig_SetupDiEnumDeviceInterfaces(
            DeviceInfoSet, DeviceInfoData, InterfaceClassGuid, MemberIndex, DeviceInterfaceData);

        // inject when: WinUSB mode, HID GUID, real scan exhausted, not yet injected
        if (!result
            && GetLastError() == ERROR_NO_MORE_ITEMS
            && Portal::g_winusb_mode
            && !g_injected_this_scan
            && GuidIsHid(InterfaceClassGuid)
            && DeviceInterfaceData)
        {
            Log("[Portal] Injecting fake HID entry at index=%lu", MemberIndex);

            DeviceInterfaceData->cbSize            = sizeof(SP_DEVICE_INTERFACE_DATA);
            DeviceInterfaceData->InterfaceClassGuid = *InterfaceClassGuid;
            DeviceInterfaceData->Flags              = SPINT_ACTIVE | SPINT_DEFAULT;
            DeviceInterfaceData->Reserved           = Portal::ENUM_SENTINEL;

            g_injected_this_scan = true;
            SetLastError(ERROR_SUCCESS);
            return TRUE; // pretend we found one more device
        }

        // log scan completion (once per unique completion index)
        if (!result && GetLastError() == ERROR_NO_MORE_ITEMS) {
            static DWORD last = 0xFFFFFFFF;
            if (MemberIndex != last) {
                Log("SetupDiEnumDeviceInterfaces: scan done, %lu device(s)", MemberIndex);
                last = MemberIndex;
            }
        }

        return result;
    }

    // ---------------------------------------------------------------------------
    // SetupDiGetDeviceInterfaceDetailW
    // ---------------------------------------------------------------------------

    using SetupDiGetDeviceInterfaceDetailW_t = BOOL(WINAPI*)(HDEVINFO, PSP_DEVICE_INTERFACE_DATA,
        PSP_DEVICE_INTERFACE_DETAIL_DATA_W, DWORD, PDWORD, PSP_DEVINFO_DATA);
    inline SetupDiGetDeviceInterfaceDetailW_t orig_SetupDiGetDeviceInterfaceDetailW = nullptr;

    inline BOOL WINAPI hook_SetupDiGetDeviceInterfaceDetailW(
        HDEVINFO DeviceInfoSet,
        PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
        PSP_DEVICE_INTERFACE_DETAIL_DATA_W DeviceInterfaceDetailData,
        DWORD DeviceInterfaceDetailDataSize,
        PDWORD RequiredSize,
        PSP_DEVINFO_DATA DeviceInfoData)
    {
        // ---- fake entry ----
        if (DeviceInterfaceData
            && DeviceInterfaceData->Reserved == Portal::ENUM_SENTINEL)
        {
            // path: "\\?\PORTAL_LIBUSB_0150"
            // C literal: L"\\\\?\\PORTAL_LIBUSB_0150"
            // offsetof(SP_DEVICE_INTERFACE_DETAIL_DATA_W, DevicePath) = 4 (sizeof DWORD)
            const DWORD path_bytes = (DWORD)((wcslen(Portal::FAKE_PATH) + 1) * sizeof(wchar_t));
            const DWORD needed = 4 + path_bytes; // cbSize field + path string

            if (RequiredSize) *RequiredSize = needed;

            if (!DeviceInterfaceDetailData || DeviceInterfaceDetailDataSize < needed) {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return FALSE; // size probe
            }

            DeviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
            wcscpy_s(DeviceInterfaceDetailData->DevicePath, DeviceInterfaceDetailDataSize / sizeof(wchar_t), Portal::FAKE_PATH);

            Log("[Portal] SetupDiGetDeviceInterfaceDetailW => fake path \"%ls\"", Portal::FAKE_PATH);
            return TRUE;
        }

        // ---- real entry ----
        BOOL result = orig_SetupDiGetDeviceInterfaceDetailW(DeviceInfoSet, DeviceInterfaceData, DeviceInterfaceDetailData,
            DeviceInterfaceDetailDataSize, RequiredSize, DeviceInfoData);

        // log portal path if found
        if (result && DeviceInterfaceDetailData && DeviceInterfaceDetailData->DevicePath[0]) {
            const wchar_t* path = DeviceInterfaceDetailData->DevicePath;
            wchar_t lower[512] = {};
            wcsncpy_s(lower, path, 511);
            _wcslwr_s(lower);
            if (wcsstr(lower, L"vid_1430") && wcsstr(lower, L"pid_0150")) {
                char narrow[512] = {};
                WideCharToMultiByte(CP_UTF8, 0, path, -1, narrow, sizeof(narrow), nullptr, nullptr);
                Log("SetupDiGetDeviceInterfaceDetailW: REAL portal path => \"%s\"", narrow);
            }
        }
        return result;
    }

    // ---------------------------------------------------------------------------
    // Hook installation
    // ---------------------------------------------------------------------------
    inline bool InitSetupApiHooks()
    {
        auto mh = [](const char* name, auto hook, auto* orig) {
            MH_STATUS s = MH_CreateHookApiEx(L"setupapi", name, hook, (void**)orig, nullptr);
            if (s != MH_OK) { Log("Failed to hook %s: %d", name, s); return false; }
            return true;
        };

        bool ok = true;
        ok &= mh("SetupDiEnumDeviceInterfaces",      &hook_SetupDiEnumDeviceInterfaces,      &orig_SetupDiEnumDeviceInterfaces);
        ok &= mh("SetupDiGetDeviceInterfaceDetailW", &hook_SetupDiGetDeviceInterfaceDetailW, &orig_SetupDiGetDeviceInterfaceDetailW);
        return ok;
    }
} // namespace ssa::SetupApiHooks