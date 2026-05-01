#pragma once

namespace
{
    using XInputGetState_t = DWORD(WINAPI*)(DWORD, uintptr_t);
    using XInputSetState_t = DWORD(WINAPI*)(DWORD, uintptr_t);
    using XInputEnable_t = void(WINAPI*)(BOOL);
    using XInputGetCapabilities_t = DWORD(WINAPI*)(DWORD, DWORD, uintptr_t);

    static XInputGetState_t XInputGetState_;
    static XInputSetState_t XInputSetState_;
    static XInputEnable_t XInputEnable_;
    static XInputGetCapabilities_t XInputGetCapabilities_;

    __declspec(dllexport) DWORD WINAPI XInputGetState(DWORD dwUserIndex, uintptr_t pVibration)
    {
        if (XInputGetState_)
        {
            return XInputGetState_(dwUserIndex, pVibration);
        }

        return ERROR_DEVICE_NOT_CONNECTED;
    }

    __declspec(dllexport) DWORD WINAPI XInputSetState(DWORD dwUserIndex, uintptr_t pVibration)
    {
        if (XInputSetState_)
        {
            return XInputSetState_(dwUserIndex, pVibration);
        }

        return ERROR_DEVICE_NOT_CONNECTED;
    }

    __declspec(dllexport) void WINAPI XInputEnable(BOOL enable)
    {
        if (XInputEnable_)
        {
            XInputEnable_(enable);
        }
    }

    __declspec(dllexport) DWORD WINAPI XInputGetCapabilities(DWORD dwUserIndex, DWORD dwFlags, uintptr_t pCapabilities)
    {
        if (XInputGetCapabilities_)
        {
            return XInputGetCapabilities_(dwUserIndex, dwFlags, pCapabilities);
        }

        return ERROR_DEVICE_NOT_CONNECTED;
    }
} // namespace
