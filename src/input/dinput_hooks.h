#pragma once
#include <Windows.h>
#include "MinHook.h"
#include "log.h"
#include "addresses.h"
#include "imgui/ui.h"

namespace ssa::DInputHooks
{
    using GetDeviceData_t = HRESULT(WINAPI*)(void*, DWORD, void*, DWORD*, DWORD);
    inline GetDeviceData_t orig_GetDeviceData_mouse = nullptr;

    inline HRESULT WINAPI hook_GetDeviceData_mouse(void* pDevice, DWORD cbObjectData, void* rgdod, DWORD* pdwInOut, DWORD dwFlags)
    {
        HRESULT hr = orig_GetDeviceData_mouse(pDevice, cbObjectData, rgdod, pdwInOut, dwFlags);
        if (SUCCEEDED(hr) && UI::Get()->IsVisible() && pdwInOut)
            *pdwInOut = 0;
        return hr;
    }

    inline bool HookDInputMouseDevice()
    {
        void* pDevice = *reinterpret_cast<void**>(GetAddress(MOUSE_DEVICE));
        if (!pDevice) {
            Log("[DInput] Mouse device pointer null");
            return false;
        }

        void** vtable = *reinterpret_cast<void***>(pDevice);
        MH_STATUS s = MH_CreateHook(vtable[10], (void*)&hook_GetDeviceData_mouse, (void**)&orig_GetDeviceData_mouse);

        if (s == MH_OK)
        {
            if (MH_EnableHook(vtable[10]) == MH_OK) {
                Log("[DInput] Hooked real mouse GetDeviceData");
                return true;
            }
        }

        Log("[DInput] Failed to hook mouse GetDeviceData");
        return false;
    }
}