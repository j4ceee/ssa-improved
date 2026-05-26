#pragma once
#include <stdio.h>
#include "log.h"
#include "MinHook.h"
#include <hidsdi.h>
#include "portal_device.h"

namespace ssa::HidHooks
{
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

    // track all preparsed data pointers so FreePreparsedData can correlate them
    constexpr int MAX_PREPARSED = 64;
    inline PHIDP_PREPARSED_DATA g_preparsed[MAX_PREPARSED] = {};
    inline int g_preparsed_count = 0;

    inline void TrackPreparsed(PHIDP_PREPARSED_DATA p)
    {
        if (!p) return;
        for (int i = 0; i < g_preparsed_count; i++)
            if (g_preparsed[i] == p) return;
        if (g_preparsed_count < MAX_PREPARSED)
            g_preparsed[g_preparsed_count++] = p;
    }

    inline bool IsTrackedPreparsed(PHIDP_PREPARSED_DATA p)
    {
        for (int i = 0; i < g_preparsed_count; i++)
            if (g_preparsed[i] == p) return true;
        return false;
    }

    // ---------------------------------------------------------------------------
    // HidD_SetOutputReport
    // ---------------------------------------------------------------------------

    using HidD_SetOutputReport_t = BOOLEAN(WINAPI*)(HANDLE, PVOID, ULONG);
    inline HidD_SetOutputReport_t orig_HidD_SetOutputReport = nullptr;

    inline BOOLEAN WINAPI hook_HidD_SetOutputReport(
        HANDLE HidDeviceObject, PVOID ReportBuffer, ULONG ReportBufferLength)
    {
        // ---- FAKE HANDLE (any kind - probe/capability/real all accept sends) ----
        if (Portal::IsFake(HidDeviceObject)) {
            if (g_config.logLevel >= LogLevel::VERBOSE)
                LogHex("[HID] [OUT - fake] SetOutputReport", (BYTE*)ReportBuffer, ReportBufferLength);
            bool ok = Portal::SendCommand((BYTE*)ReportBuffer, ReportBufferLength);
            return ok ? TRUE : FALSE;
        }

        // ---- REAL HID HANDLE ----
        if (g_config.logLevel >= LogLevel::VERBOSE)
            LogHex("[HID] [OUT] SetOutputReport", (BYTE*)ReportBuffer, ReportBufferLength);

        BOOLEAN result = orig_HidD_SetOutputReport(HidDeviceObject, ReportBuffer, ReportBufferLength);
        if (!result) Log("[HID] SetOutputReport FAILED (error=%lu)", GetLastError());
        return result;
    }

    // ---------------------------------------------------------------------------
    // HidD_GetAttributes
    // ---------------------------------------------------------------------------

    using HidD_GetAttributes_t = BOOLEAN(WINAPI*)(HANDLE, PHIDD_ATTRIBUTES);
    inline HidD_GetAttributes_t orig_HidD_GetAttributes = nullptr;

    inline BOOLEAN WINAPI hook_HidD_GetAttributes(
        HANDLE HidDeviceObject, PHIDD_ATTRIBUTES Attributes)
    {
        // ---- FAKE HANDLE ----
        if (Portal::IsFake(HidDeviceObject)) {
            if (!Attributes) return FALSE;
            Attributes->Size          = sizeof(HIDD_ATTRIBUTES);
            Attributes->VendorID      = Portal::VID;
            Attributes->ProductID     = Portal::PID;
            Attributes->VersionNumber = 0x0100;
            LogDebug("HidD_GetAttributes(fake=%p) => VID=0x1430 PID=0x0150", HidDeviceObject);
            return TRUE;
        }

        // ---- REAL HID HANDLE ----
        BOOLEAN result = orig_HidD_GetAttributes(HidDeviceObject, Attributes);
        if (result && Attributes && Attributes->VendorID == 0x1430 && Attributes->ProductID == 0x0150)
            LogDebug("HidD_GetAttributes(handle=%p) => PORTAL VID=0x1430 PID=0x0150", HidDeviceObject);
        return result;
    }

    // ---------------------------------------------------------------------------
    // HidD_GetPreparsedData
    // ---------------------------------------------------------------------------

    using HidD_GetPreparsedData_t = BOOLEAN(WINAPI*)(HANDLE, PHIDP_PREPARSED_DATA*);
    inline HidD_GetPreparsedData_t orig_HidD_GetPreparsedData = nullptr;

    inline BOOLEAN WINAPI hook_HidD_GetPreparsedData(
        HANDLE dev, PHIDP_PREPARSED_DATA* data)
    {
        // ---- FAKE HANDLE ----
        if (Portal::IsFake(dev)) {
            if (!data) return FALSE;
            *data = Portal::PREPARSED_SENTINEL;
            LogDebug("HidD_GetPreparsedData(fake=%p) => sentinel=%p", dev, Portal::PREPARSED_SENTINEL);
            return TRUE;
        }

        // ---- REAL HID HANDLE ----
        BOOLEAN r = orig_HidD_GetPreparsedData(dev, data);
        if (r && data && *data) {
            TrackPreparsed(*data);
            LogDebug("HidD_GetPreparsedData(handle=%p) => %p (tracking %d)",
                dev, *data, g_preparsed_count);
        }
        return r;
    }

    // ---------------------------------------------------------------------------
    // HidP_GetCaps
    // ---------------------------------------------------------------------------

    using HidP_GetCaps_t = NTSTATUS(WINAPI*)(PHIDP_PREPARSED_DATA, PHIDP_CAPS);
    inline HidP_GetCaps_t orig_HidP_GetCaps = nullptr;

    inline NTSTATUS WINAPI hook_HidP_GetCaps(
        PHIDP_PREPARSED_DATA PreparsedData, PHIDP_CAPS Capabilities)
    {
        // ---- OUR SENTINEL ----
        if (PreparsedData == Portal::PREPARSED_SENTINEL) {
            if (!Capabilities) return HIDP_STATUS_INVALID_PREPARSED_DATA;
            ZeroMemory(Capabilities, sizeof(HIDP_CAPS));
            Capabilities->InputReportByteLength  = 33; // 1 report ID + 32 data
            Capabilities->OutputReportByteLength = 33;
            LogDebug("HidP_GetCaps(sentinel) => Input=33 Output=33");
            return HIDP_STATUS_SUCCESS;
        }

        // ---- REAL ----
        NTSTATUS result = orig_HidP_GetCaps(PreparsedData, Capabilities);
        if (result == HIDP_STATUS_SUCCESS && Capabilities)
            LogDebug("HidP_GetCaps(preparsed=%p%s) => Input=%u Output=%u",
                PreparsedData,
                IsTrackedPreparsed(PreparsedData) ? " [tracked]" : "",
                Capabilities->InputReportByteLength,
                Capabilities->OutputReportByteLength);
        return result;
    }

    // ---------------------------------------------------------------------------
    // HidD_FreePreparsedData
    // ---------------------------------------------------------------------------

    using HidD_FreePreparsedData_t = BOOLEAN(WINAPI*)(PHIDP_PREPARSED_DATA);
    inline HidD_FreePreparsedData_t orig_HidD_FreePreparsedData = nullptr;

    inline BOOLEAN WINAPI hook_HidD_FreePreparsedData(PHIDP_PREPARSED_DATA PreparsedData)
    {
        if (PreparsedData == Portal::PREPARSED_SENTINEL) {
            // no-op: don't free fake pointer
            return TRUE;
        }
        if (IsTrackedPreparsed(PreparsedData))
            LogDebug("HidD_FreePreparsedData(tracked=%p)", PreparsedData);
        return orig_HidD_FreePreparsedData(PreparsedData);
    }

    // ---------------------------------------------------------------------------
    // HidD_SetNumInputBuffers - not called by portal.dll
    // ---------------------------------------------------------------------------

    using HidD_SetNumInputBuffers_t = BOOLEAN(WINAPI*)(HANDLE, ULONG);
    inline HidD_SetNumInputBuffers_t orig_HidD_SetNumInputBuffers = nullptr;

    inline BOOLEAN WINAPI hook_HidD_SetNumInputBuffers(HANDLE dev, ULONG n)
    {
        if (Portal::IsFake(dev)) {
            LogDebug("HidD_SetNumInputBuffers(fake, %lu) => TRUE (no-op)", n);
            return TRUE;
        }
        LogDebug("HidD_SetNumInputBuffers(handle=%p, %lu)", dev, n);
        return orig_HidD_SetNumInputBuffers(dev, n);
    }

    // ---------------------------------------------------------------------------
    // Hook installation
    // ---------------------------------------------------------------------------
    inline bool InitHidHooks()
    {
        bool ok = true;
        auto hook = [&](const char* name, auto fn, auto* orig, bool required = false) {
            MH_STATUS s = MH_CreateHookApiEx(L"hid", name, fn, (void**)orig, nullptr);
            if (s != MH_OK) {
                Log("Hook failed [%s]: %d", name, s);
                if (required) ok = false;
            }
        };

        hook("HidD_SetOutputReport",    &hook_HidD_SetOutputReport,    &orig_HidD_SetOutputReport, true);
        hook("HidD_GetAttributes",      &hook_HidD_GetAttributes,      &orig_HidD_GetAttributes);
        hook("HidD_GetPreparsedData",   &hook_HidD_GetPreparsedData,   &orig_HidD_GetPreparsedData);
        hook("HidD_FreePreparsedData",  &hook_HidD_FreePreparsedData,  &orig_HidD_FreePreparsedData);
        hook("HidD_SetNumInputBuffers", &hook_HidD_SetNumInputBuffers, &orig_HidD_SetNumInputBuffers);
        hook("HidP_GetCaps",            &hook_HidP_GetCaps,            &orig_HidP_GetCaps);

        return ok;
    }
} // namespace ssa::HidHooks