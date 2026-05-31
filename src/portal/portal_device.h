#pragma once
#include <Windows.h>
#include <hidsdi.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <setupapi.h>

#include "libusb.h"
#include "log.h"
#include "config.h"
#include "IPortalBackend.h"
#include "./backend/WinUSBBackend.h"
#include "./backend/ProxyBackend.h"
#include "./backend/EmulatedBackend.h"

/**
 * Portal shim layer
 *
 * Owns...
 * - fake-handle pool
 * - reader thread
 * - a single IPortalBackend instance selected at startup
 *
 * All hooks route portal I/O through wrappers at the bottom of this file, they never speak to a backend directly
 *
 * Backend selection order (first available wins):
 * - <b>EmulatedBackend</b> if enabled in the config
 * - <b>ProxyBackend</b> (Wine / Proton detected + proxy daemon reachable)
 * - <b>HID pass-through</b> (portal found in HID enumeration, HID driver installed → use portal directly)
 * - <b>WinUSBBackend</b> (portal found via libusb & openable)
 * - <b>EmulatedBackend</b> if no other portal found TODO
 *
 * g_inject_mode controls SetupAPI injection and fake-handle allocation:
 * - <code>true</code>: portal.dll sees FAKE_PATH and gets fake handles, all I/O is routed through the active backend
 * - <code>false</code>: portal.dll opens the real device directly, hooks log and pass through
 */
namespace ssa::Portal
{
    // ---------------------------------------------------------------------------
    // Constants
    // ---------------------------------------------------------------------------

    constexpr uint16_t VID = 0x1430;
    constexpr uint16_t PID = 0x0150;
    constexpr ULONG STATUS_PENDING_VAL = 0x00000103UL; // NTSTATUS STATUS_PENDING

    // fake device path injected into SetupAPI enumeration in inject mode
    constexpr wchar_t FAKE_PATH[] = L"\\\\?\\PORTAL_LIBUSB_0150";

    // sentinel returned by HidD_GetPreparsedData for fake handles
    inline const auto PREPARSED_SENTINEL = reinterpret_cast<PHIDP_PREPARSED_DATA>(0x50524c00UL);

    // sentinel stored in SP_DEVICE_INTERFACE_DATA (reserved to identify injected entries)
    constexpr ULONG_PTR ENUM_SENTINEL = 0xBAD10150UL;

    // ---------------------------------------------------------------------------
    // Active backend + injection mode
    // ---------------------------------------------------------------------------

    inline IPortalBackend* g_backend = nullptr;
    inline bool g_inject_mode = false;
    inline bool g_init_attempted = false;

    // ---------------------------------------------------------------------------
    // Fake handle pool
    //
    // Real Win32 event handles are used as fake portal handles so CloseHandle
    // works naturally and the handle value is a valid non-null pointer.
    // ---------------------------------------------------------------------------

    enum class HKind { NONE, PROBE, CAPABILITY, REAL };

    struct FakeEntry
    {
        HANDLE h = nullptr;
        HKind kind = HKind::NONE;
    };

    constexpr int MAX_FAKE = 8;
    inline FakeEntry g_fake[MAX_FAKE] = {};
    inline std::mutex g_fake_mtx;

    inline HANDLE AllocFake(HKind kind)
    {
        HANDLE h = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if (!h || h == INVALID_HANDLE_VALUE)
        {
            Log("[Portal] AllocFake: CreateEvent failed");
            return INVALID_HANDLE_VALUE;
        }
        std::lock_guard lk(g_fake_mtx);
        for (auto& e : g_fake)
        {
            if (!e.h)
            {
                e.h = h;
                e.kind = kind;
                return h;
            }
        }
        CloseHandle(h);
        Log("[Portal] AllocFake: pool exhausted");
        return INVALID_HANDLE_VALUE;
    }

    inline HKind GetKind(HANDLE h)
    {
        if (!h || h == INVALID_HANDLE_VALUE) return HKind::NONE;
        std::lock_guard<std::mutex> lk(g_fake_mtx);
        for (auto& e : g_fake) if (e.h == h) return e.kind;
        return HKind::NONE;
    }

    inline bool IsFake(HANDLE h) { return GetKind(h) != HKind::NONE; }

    inline void FreeFake(HANDLE h)
    {
        std::lock_guard<std::mutex> lk(g_fake_mtx);
        for (auto& e : g_fake)
        {
            if (e.h == h)
            {
                CloseHandle(h);
                e = {};
                return;
            }
        }
    }

    // ---------------------------------------------------------------------------
    // Reader I/O state (one instance, owned by the shim)
    // ---------------------------------------------------------------------------

    struct IO
    {
        HANDLE owner = nullptr; // fake REAL handle that last armed a read
        std::thread thread;
        std::mutex mtx;
        std::condition_variable cv;
        bool arm = false; // set by ArmRead, cleared by reader thread
        bool stop = false; // set by CloseUSB / Shutdown
        BYTE* buf = nullptr; // output buffer for the current read
        LPOVERLAPPED ovlp = nullptr; // OVERLAPPED for the current read
    };

    inline IO* g_io = nullptr;

    // ---------------------------------------------------------------------------
    // Global portal state
    // ---------------------------------------------------------------------------
    struct PortalState
    {
        struct Color
        {
            uint8_t r = 0, g = 0, b = 0;
        };

        Color color;
        bool slotPresent[16] = {};
        uint16_t slotFigureId[16] = {}; // 0 = not yet identified
        uint16_t slotVariantId[16] = {};
        std::mutex mtx;
    };

    struct SlotInfo
    {
        bool present;
        uint16_t figureId;
        uint16_t variantId;
    };

    inline PortalState g_portalState;

    // called from SendCommand() and hook_HidD_SetOutputReport (real HID path)
    inline void SniffOutgoing(const BYTE* buf, size_t len)
    {
        if (!buf) return;

        if (len >= 5 && buf[1] == 'C')
        {
            // C: [0x00, 'C', R, G, B]
            std::lock_guard lk(g_portalState.mtx);
            g_portalState.color = {buf[2], buf[3], buf[4]};
        }
        else if (len >= 7 && buf[1] == 'J')
        {
            // J: [0x00, 'J', sides, R, G, B, dur_lo, dur_hi]
            // sides is the ring bitfield, R/G/B start at buf[3]
            std::lock_guard lk(g_portalState.mtx);
            g_portalState.color = {buf[3], buf[4], buf[5]};
        }
    }

    // called from ReaderThread after every successful ReadResponse()
    inline void SniffIncoming(const BYTE* buf, int len)
    {
        if (!buf || len < 3) return;

        if (buf[1] == 0x53 && len >= 6) // 'S' status packet
        {
            uint32_t status = static_cast<uint32_t>(buf[2])
                | static_cast<uint32_t>(buf[3]) << 8
                | static_cast<uint32_t>(buf[4]) << 16
                | static_cast<uint32_t>(buf[5]) << 24;

            std::lock_guard lk(g_portalState.mtx);
            for (int i = 0; i < 16; i++, status >>= 2)
            {
                bool nowPresent = (status & 1) != 0;
                if (!nowPresent && g_portalState.slotPresent[i])
                {
                    // figure removed -> clear stored identity
                    g_portalState.slotFigureId[i] = 0;
                    g_portalState.slotVariantId[i] = 0;
                }
                g_portalState.slotPresent[i] = nowPresent;
            }
        }
        else if (buf[1] == 0x51 && len >= 20) // 'Q' query response
        {
            // buf[2]: bit4 = figure present, bits 0-3 = slot index
            // buf[3]: block number
            // buf[4..19]: 16 bytes of raw block data
            //
            // Block 1 of the figure data (global bytes 16-31) holds:
            //  figureId  at block offset  0 (global byte 0x10)
            //  variantId at block offset 12 (global byte 0x1C)
            bool present = (buf[2] & 0x10) != 0;
            uint8_t skyNum = buf[2] & 0xF;
            uint8_t block = buf[3];

            if (present && skyNum < 16 && block == 1)
            {
                uint16_t fid = static_cast<uint16_t>(buf[4]) | static_cast<uint16_t>(buf[5]) << 8;
                uint16_t vid = static_cast<uint16_t>(buf[16]) | static_cast<uint16_t>(buf[17]) << 8;
                std::lock_guard lk(g_portalState.mtx);
                g_portalState.slotFigureId[skyNum] = fid;
                g_portalState.slotVariantId[skyNum] = vid;
            }
        }
    }

    inline PortalState::Color GetPortalColor()
    {
        std::lock_guard lk(g_portalState.mtx);
        return g_portalState.color;
    }

    inline std::array<SlotInfo, 16> GetSlotInfo()
    {
        std::lock_guard lk(g_portalState.mtx);
        std::array<SlotInfo, 16> r{};
        for (int i = 0; i < 16; i++)
            r[i] = { g_portalState.slotPresent[i],
                     g_portalState.slotFigureId[i],
                     g_portalState.slotVariantId[i] };
        return r;
    }

    // ---------------------------------------------------------------------------
    // Reader thread
    //
    // Waits for ArmRead() to signal arm, then calls backend->ReadResponse()
    // in a retry loop (timeout → retry) until a packet arrives, an error
    // occurs, or stop is set.  Signals the OVERLAPPED event on success.
    // ---------------------------------------------------------------------------
    inline void ReaderThread(IO* io)
    {
        Log("[Portal] Reader thread started (%s)",
            g_backend ? g_backend->Name() : "?");

        while (true)
        {
            BYTE* buf = nullptr;
            LPOVERLAPPED ovlp = nullptr;

            // wait for ArmRead to signal
            {
                std::unique_lock lk(io->mtx);
                io->cv.wait(lk, [io] { return io->arm || io->stop; });
                if (io->stop) break;
                io->arm = false;
                buf = io->buf;
                ovlp = io->ovlp;
            }

            if (!buf || !ovlp || !g_backend) continue;

            // retry until we get a packet, hit an error or are told to stop
            while (!io->stop)
            {
                int got = g_backend->ReadResponse(buf, 33, 500);
                if (io->stop) goto done;

                if (got > 0)
                {
                    SniffIncoming(buf, got);

                    LogDebug("[IN] (%s) cmd=0x%02X [%02X %02X %02X %02X]",
                             g_backend->Name(),
                             buf[1], buf[2], buf[3], buf[4], buf[5]);

                    // clear the armed state before signalling so a new ArmRead can safely update buf / ovlp while the game is handling
                    // the completed overlapped op
                    {
                        std::lock_guard<std::mutex> lk(io->mtx);
                        io->buf = nullptr;
                        io->ovlp = nullptr;
                    }

                    ovlp->InternalHigh = static_cast<ULONG_PTR>(got);
                    ovlp->Internal = 0;
                    if (ovlp->hEvent && ovlp->hEvent != INVALID_HANDLE_VALUE)
                        SetEvent(ovlp->hEvent);
                    break; // back to outer arm-wait
                }

                if (got < 0)
                {
                    Log("[Portal] ReadResponse error (%s)", g_backend->Name());
                    goto done;
                }
                // got == 0: timeout -> keep retrying
            }
        }

    done:
        Log("[Portal] Reader thread exiting");
    }

    // ---------------------------------------------------------------------------
    // Backend selection
    // ---------------------------------------------------------------------------

    [[nodiscard]] inline bool FindHidPortal() {
        GUID guid; HidD_GetHidGuid(&guid);
        HDEVINFO dev = SetupDiGetClassDevsW(&guid, nullptr, nullptr,
                                            DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
        if (dev == INVALID_HANDLE_VALUE) return false;

        SP_DEVICE_INTERFACE_DATA iface{ sizeof(SP_DEVICE_INTERFACE_DATA) };
        bool found = false;
        for (DWORD i = 0; SetupDiEnumDeviceInterfaces(dev, nullptr, &guid, i, &iface); ++i) {
            DWORD needed = 0;
            SetupDiGetDeviceInterfaceDetailW(dev, &iface, nullptr, 0, &needed, nullptr);
            std::vector<BYTE> buf(needed);
            auto* detail = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(buf.data());
            detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
            if (!SetupDiGetDeviceInterfaceDetailW(dev, &iface, detail, needed, nullptr, nullptr)) continue;
            wchar_t lower[512]{}; wcsncpy_s(lower, detail->DevicePath, 511); _wcslwr_s(lower);
            if (wcsstr(lower, L"vid_1430") && wcsstr(lower, L"pid_0150")) { found = true; break; }
        }
        SetupDiDestroyDeviceInfoList(dev);
        return found;
    }

    inline bool Init()
    {
        // --- 1. Emulated Portal (config flag - highest priority) ---
        if (g_config.emulatedPortalStartup)
        {
            g_backend = new Backend::EmulatedBackend();
            g_inject_mode = true;
            Log("[Portal] Backend selected: Emulated (inject mode active)");
            return true;
        }

        Log("[Portal] Emulated portal not selected or failed to initialize, trying next backend...");

        // --- 2. Proxy (Wine + native daemon) ---
        {
            // Wine / Proton detection
            bool is_wine = GetModuleHandleW(L"ntdll.dll") &&
                GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "wine_get_version");

            auto* proxy = new Backend::ProxyBackend();
            if (is_wine)
            {
                Log("[Portal] Running under Wine/Proton");

                if (proxy->IsAvailable())
                {
                    g_backend = proxy;
                    g_inject_mode = true;
                    Log("[Portal] Backend selected: Proxy (inject mode active)");
                    return true;
                }
                delete proxy;
                Log("[Portal] Proxy backend not available - is the proxy daemon running?");
                return false; // exit at this point, under wine there is no other way to play the game
            }
            delete proxy;
        }

        Log("[Portal] Proxy backend not selected (not Linux), trying next backend...");

        // --- 3. HID (pass-through - inject mode stays false) ---
        {
            if (FindHidPortal()) {
                g_inject_mode = false;  // portal.dll uses the real device directly
                Log("[Portal] HID portal found - pass-through mode (no backend)");
                return true;
            }
        }

        Log("[Portal] HID backend not available or no portal with HID driver found, trying next backend...");

        // --- 4. WinUSB (libusb) ---
        {
            libusb_context* ctx = nullptr;
            if (libusb_init(&ctx) == LIBUSB_SUCCESS)
            {
                libusb_device** list = nullptr;
                ssize_t count = libusb_get_device_list(ctx, &list);
                bool winusb_active = false;

                for (ssize_t i = 0; i < count; i++)
                {
                    libusb_device_descriptor desc{};
                    if (libusb_get_device_descriptor(list[i], &desc) == LIBUSB_SUCCESS
                        && desc.idVendor == VID && desc.idProduct == PID)
                    {
                        // portal physically found -> test if WinUSB driver is active
                        libusb_device_handle* test_handle = nullptr;
                        if (libusb_open(list[i], &test_handle) == LIBUSB_SUCCESS)
                        {
                            if (libusb_claim_interface(test_handle, 0) == LIBUSB_SUCCESS)
                            {
                                libusb_release_interface(test_handle, 0);

                                // Claim successful - WinUSB is the active driver
                                g_backend = new Backend::WinUSBBackend(ctx, libusb_ref_device(list[i]));
                                g_inject_mode = true;
                                winusb_active = true;
                                Log("[Portal] Backend selected: WinUSB (inject mode active)");
                            }
                            libusb_close(test_handle);
                        }
                    }
                    if (winusb_active) break;
                }
                libusb_free_device_list(list, 1);

                if (winusb_active) return true;

                // if we get here, either no portal is plugged in, or it is using the HID driver
                // clean up context and let the factory fall back to HID
                libusb_exit(ctx);
            }
        }

        Log("[Portal] No portal backend available");
        return false;
    }

    // ---------------------------------------------------------------------------
    // Device lifecycle (called from kernel.h hooks)
    // ---------------------------------------------------------------------------

    // called when portal.dll opens the REAL fake handle (CreateFileW with FAKE_PATH and FILE_FLAG_OVERLAPPED).
    // opens the backend and starts the reader thread
    inline bool OpenUSB()
    {
        if (g_io)
        {
            Log("[Portal] OpenUSB: already open, ignoring duplicate");
            return true;
        }
        if (!g_backend || !g_backend->Open())
        {
            Log("[Portal] OpenUSB: backend->Open() failed");
            return false;
        }
        g_io = new IO{};
        g_io->thread = std::thread(ReaderThread, g_io);
        Log("[Portal] Opened (%s)", g_backend->Name());
        return true;
    }

    // called when portal.dll closes the REAL fake handle
    // some backends (Proxy) stay alive across close/reopen cycles
    inline void CloseUSB()
    {
        if (!g_io || !g_backend) return;

        if (g_backend->PersistAcrossClose())
        {
            Log("[Portal] CloseUSB: %s persists across close, skipping", g_backend->Name());
            return;
        }

        {
            std::lock_guard lk(g_io->mtx);
            g_io->stop = true;
        }
        g_io->cv.notify_all();
        g_backend->Close();
        if (g_io->thread.joinable()) g_io->thread.join();
        delete g_io;
        g_io = nullptr;
        Log("[Portal] Closed (%s)", g_backend->Name());
    }

    // full teardown (called from DllMain on DLL_PROCESS_DETACH)
    inline void Shutdown()
    {
        if (g_io)
        {
            {
                std::lock_guard lk(g_io->mtx);
                g_io->stop = true;
            }
            g_io->cv.notify_all();
            if (g_io->thread.joinable()) g_io->thread.join();
            delete g_io;
            g_io = nullptr;
        }
        if (g_backend)
        {
            g_backend->Close();
            delete g_backend;
            g_backend = nullptr;
        }
        Log("[Portal] Shutdown complete");
    }

    // ---------------------------------------------------------------------------
    // I/O wrappers (called from hooks - do not call backend directly)
    // ---------------------------------------------------------------------------

    // called from hook_ReadFile when portal.dll arms a read on the REAL fake handle
    // - tries a synchronous fast-path delivery first (for ProxyBackend's pre-buffered packet)
    // - falls back to notifying the reader thread.
    inline void ArmRead(BYTE* buf, LPOVERLAPPED ovlp, HANDLE owner)
    {
        if (!g_io) return;

        if (ovlp)
        {
            ovlp->Internal = static_cast<ULONG_PTR>(STATUS_PENDING_VAL);
            ovlp->InternalHigh = 0;
        }

        // fast path: backends that pre-buffer packets (ProxyBackend) can satisfy the read synchronously so the event is
        // signalled before portal.dll's WaitForSingleObject(event, 0) fires
        if (g_backend && ovlp)
        {
            int got = g_backend->TryReadImmediate(buf, 33);
            if (got > 0)
            {
                ovlp->InternalHigh = got;
                ovlp->Internal = 0;
                if (ovlp->hEvent && ovlp->hEvent != INVALID_HANDLE_VALUE)
                    SetEvent(ovlp->hEvent);
                return;
            }
        }

        // async path: wake the reader thread
        {
            std::lock_guard lk(g_io->mtx);
            g_io->buf = buf;
            g_io->ovlp = ovlp;
            g_io->arm = true;
            g_io->owner = owner;
        }
        g_io->cv.notify_one();
    }

    // called from hook_HidD_SetOutputReport for fake handles
    // buf[0] = report ID (0x00), buf[1..33] = command, len = 34.
    inline bool SendCommand(const BYTE* buf, ULONG len)
    {
        SniffOutgoing(buf, static_cast<size_t>(len));
        return g_backend && g_backend->SendCommand(buf, static_cast<size_t>(len));
    }

    // ---------------------------------------------------------------------------
    // Path helper
    // ---------------------------------------------------------------------------

    inline bool IsFakePath(LPCWSTR path)
    {
        if (!path) return false;
        wchar_t lower[MAX_PATH] = {};
        wcsncpy_s(lower, path, MAX_PATH - 1);
        _wcslwr_s(lower);
        return wcsstr(lower, L"portal_libusb_0150") != nullptr;
    }

    // ---------------------------------------------------------------------------
    // Other helpers
    // ---------------------------------------------------------------------------

    /**
     * @return active EmulatedBackend if the emulated portal is selected, else nullptr
     */
    inline Backend::EmulatedBackend* GetEmulated()
    {
        return dynamic_cast<Backend::EmulatedBackend*>(g_backend);
    }
} // namespace ssa::Portal
