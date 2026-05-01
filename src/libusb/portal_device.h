#pragma once
#include <winsock2.h>
#include <Windows.h>
#include <hidsdi.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

#include "libusb.h"
#include "log.h"

// ---------------------------------------------------------------------------
// Portal device abstraction layer
// - owns the libusb handle, reader thread, and fake handle pool
// - all other hooks call into this namespace to route portal.dll's API calls to
//   libusb when the WinUSB driver is installed
// ---------------------------------------------------------------------------
namespace ssa::Portal {

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

constexpr uint16_t VID = 0x1430;
constexpr uint16_t PID = 0x0150;
constexpr uint8_t  EP_IN     = 0x81;
constexpr int      IFACE     = 0;
constexpr ULONG    STATUS_PENDING_VAL = 0x00000103UL; // NTSTATUS STATUS_PENDING

// fake device path we inject into SetupAPI enumeration
constexpr wchar_t FAKE_PATH[] = L"\\\\?\\PORTAL_LIBUSB_0150";

// sentinel for PHIDP_PREPARSED_DATA (will be passed back to HidP_GetCaps & HidD_FreePreparsedData)
inline const PHIDP_PREPARSED_DATA PREPARSED_SENTINEL =
    reinterpret_cast<PHIDP_PREPARSED_DATA>(static_cast<ULONG_PTR>(0x50524c00UL));

// sentinel in SP_DEVICE_INTERFACE_DATA.Reserved to mark injected entry
constexpr ULONG_PTR ENUM_SENTINEL = 0xBAD10150UL;

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------

inline bool             g_winusb_mode = false;
inline libusb_context*  g_ctx         = nullptr;
inline libusb_device*   g_device      = nullptr; // kept alive with libusb_ref_device

// --- Fake handle pool ---
// return real Win32 event handles as fake portal handles
// -> CloseHandle works naturally and the handle value is valid

enum class HKind { NONE, PROBE, CAPABILITY, REAL };

struct FakeEntry {
    HANDLE h    = nullptr;
    HKind  kind = HKind::NONE;
};

constexpr int MAX_FAKE = 8;
inline FakeEntry   g_fake[MAX_FAKE] = {};
inline std::mutex  g_fake_mtx;

// --- Per-REAL-handle I/O state ---

struct IO {
    libusb_device_handle* usb = nullptr;

    HANDLE                owner = nullptr; // handle that last called ArmRead

    // reader thread
    std::thread           thread;
    std::mutex            mtx;
    std::condition_variable cv;
    bool                  arm  = false;   // set by ArmRead, cleared by reader
    bool                  stop = false;   // set by CloseUSB

    // current pending read (set by ArmRead)
    BYTE*         buf  = nullptr;
    LPOVERLAPPED  ovlp = nullptr;

    // rolling packet buffer for proxy mode
    // always-on reader continuously fills this so ArmRead can deliver packets synchronously (without waiting for the next recv() call)
    uint8_t       pkt_buf[33] = {};
    bool          pkt_ready   = false;
};

// only one REAL handle exists at a time.
inline IO* g_io = nullptr;

inline SOCKET g_proxy_sock = INVALID_SOCKET;
inline bool   g_is_wine    = false;

inline bool IsRunningUnderWine()
{
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    return ntdll && GetProcAddress(ntdll, "wine_get_version") != nullptr;
}

inline bool ConnectToProxy()
{
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return false;

    SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) return false;

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(8764);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        closesocket(s);
        return false;
    }
    g_proxy_sock = s;
    return true;
}

// ---------------------------------------------------------------------------
// Fake handle helpers
// ---------------------------------------------------------------------------

inline HANDLE AllocFake(HKind kind)
{
    HANDLE h = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (!h || h == INVALID_HANDLE_VALUE) {
        Log("[Portal] AllocFake: CreateEvent failed");
        return INVALID_HANDLE_VALUE;
    }
    std::lock_guard<std::mutex> lk(g_fake_mtx);
    for (auto& e : g_fake) {
        if (!e.h) { e.h = h; e.kind = kind; return h; }
    }
    CloseHandle(h);
    Log("[Portal] AllocFake: pool exhausted!");
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
    for (auto& e : g_fake) {
        if (e.h == h) {
            CloseHandle(h);
            e = {};
            return;
        }
    }
}

// ---------------------------------------------------------------------------
// Reader thread
// ---------------------------------------------------------------------------
inline void ReaderThread(IO* io)
{
    const bool proxy = (g_proxy_sock != INVALID_SOCKET);
    Log("[Portal] Reader thread started (%s)", proxy ? "proxy" : "libusb");

    // ---- Proxy mode: always-on reader ----
    // - continuously recv from the proxy socket without waiting for ArmRead
    // - each received packet is saved to io->pkt_buf so that ArmRead can deliver it synchronously (before the WaitForSingleObject(event, 0) call)
    // - necessary because portal.dll under Wine opens a REAL handle, calls ReadFile, immediately polls WaitForSingleObject(timeout=0), then closes the handle (within <1ms)
    // - now the delivery happens in ArmRead itself
    if (proxy) {
        DWORD timeout_ms = 100;
        setsockopt(g_proxy_sock, SOL_SOCKET, SO_RCVTIMEO,
            reinterpret_cast<char*>(&timeout_ms), sizeof(timeout_ms));

        while (!io->stop) {
            uint8_t pkt[33] = {};
            int received = 0;

            while (received < 33 && !io->stop) {
                int r = recv(g_proxy_sock,
                    reinterpret_cast<char*>(pkt) + received,
                    33 - received, 0);
                if (r <= 0) {
                    int err = WSAGetLastError();
                    if (err == WSAETIMEDOUT || err == WSAEWOULDBLOCK) break;
                    if (!io->stop)
                        Log("[Portal] Proxy recv error: %d", err);
                    goto done;
                }
                received += r;
            }

            if (received < 33 || io->stop) continue;
            if (pkt[0] != 0x02) continue;

            {
                std::lock_guard<std::mutex> lk(io->mtx);

                // always keep a rolling buffer of the most recent packet
                memcpy(io->pkt_buf, pkt, 33);
                io->pkt_ready = true;

                // if an async read is armed, deliver now
                if (io->buf && io->ovlp) {
                    io->buf[0] = 0x00;
                    memcpy(io->buf + 1, pkt + 1, 32);
                    io->ovlp->InternalHigh = 33;
                    io->ovlp->Internal     = 0;
                    HANDLE ev = io->ovlp->hEvent;
                    io->buf  = nullptr; // clear so we don't re-deliver without a new ArmRead
                    io->ovlp = nullptr;
                    if (ev && ev != INVALID_HANDLE_VALUE)
                        SetEvent(ev);

                    // -- Logging
                    uint8_t cmd = pkt[1];
                    static uint8_t last_cmd = 0;
                    if (cmd != last_cmd) {
                        LogDebug("[IN] (Proxy) cmd=0x%02X [%02X %02X %02X %02X]",
                            cmd, pkt[2], pkt[3], pkt[4], pkt[5]);
                        last_cmd = cmd;
                    }
                }
            }
        }
        goto done;
    }

    // ---- libusb mode: arm-wait outer loop (unchanged) ----
    while (true) {
        {
            std::unique_lock<std::mutex> lk(io->mtx);
            io->cv.wait(lk, [io] { return io->arm || io->stop; });
            if (io->stop) break;
            io->arm = false;
        }

        while (true) {
            if (io->stop) goto done;

            uint8_t raw[32] = {};
            int     got     = 0;
            int r = libusb_interrupt_transfer(io->usb, EP_IN, raw, sizeof(raw), &got, 500);
            if (io->stop) goto done;
            if (r == LIBUSB_SUCCESS && got > 0 && io->buf && io->ovlp) {
                io->buf[0] = 0x00;
                memcpy(io->buf + 1, raw, got);
                io->ovlp->InternalHigh = static_cast<ULONG_PTR>(1 + got);
                io->ovlp->Internal     = 0;
                if (io->ovlp->hEvent && io->ovlp->hEvent != INVALID_HANDLE_VALUE)
                    SetEvent(io->ovlp->hEvent);

                // -- Logging
                uint8_t cmd = io->buf[1];
                static uint8_t last_cmd = 0;
                if (cmd != last_cmd) {
                    LogDebug("[IN] (libusb) cmd=0x%02X [%02X %02X %02X %02X]",
                        cmd, io->buf[2], io->buf[3], io->buf[4], io->buf[5]);
                    last_cmd = cmd;
                }

                break;
            } else if (r != LIBUSB_ERROR_TIMEOUT) {
                Log("[Portal] Transfer error: %s", libusb_error_name(r));
                goto done;
            }
        }
    }
done:
    Log("[Portal] Reader thread exiting");
}

// ---------------------------------------------------------------------------
// Device lifecycle
// ---------------------------------------------------------------------------

inline bool Init()
{
    // -- handle Linux --

    g_is_wine = IsRunningUnderWine();

    if (g_is_wine) {
        Log("[Portal] Running under Wine/Proton");
        if (ConnectToProxy()) {
            g_winusb_mode = true; // activate fake handle injection
            Log("[Portal] Proxy connected on :8764 - proxy mode active");
        } else {
            g_winusb_mode = false;
            Log("[Portal] No proxy on :8764 - pass-through mode (writes will fail)");
            Log("[Portal] Run portal_proxy.py before launching the game");
        }
        return true;
    }

    // -- handle Windows --

    // try libusb for WinUSB driver scenario
    if (libusb_init(&g_ctx) != LIBUSB_SUCCESS) {
        Log("[Portal] libusb_init failed");
        return false;
    }

    libusb_device** list = nullptr;
    ssize_t count = libusb_get_device_list(g_ctx, &list);
    for (ssize_t i = 0; i < count; i++)
    {
        libusb_device_descriptor desc{};
        if (libusb_get_device_descriptor(list[i], &desc) == LIBUSB_SUCCESS
            && desc.idVendor == VID && desc.idProduct == PID)
        {
            g_device = libusb_ref_device(list[i]);
            g_winusb_mode = true;
            break;
        }
    }
    libusb_free_device_list(list, 1);

    if (g_winusb_mode)
        Log("[Portal] Portal found via libusb - WinUSB injection active");
    else
        Log("[Portal] Portal not visible via libusb - pass-through mode");

    return true;
}

// open the device and start the reader thread
inline bool OpenUSB()
{
    if (g_io) {
        Log("[Portal] OpenUSB called but g_io already exists - ignoring duplicate open");
        return true;
    }

    // ---- proxy mode (Wine + native daemon) ----
    if (g_proxy_sock != INVALID_SOCKET) {
        // verify the socket is still alive with a zero-byte send
        char probe = 0;
        int test = send(g_proxy_sock, &probe, 0, 0);
        if (test == SOCKET_ERROR)
        {
            int err = WSAGetLastError();
            Log("[Portal] OpenUSB: proxy socket is dead (err=%d) - reconnecting", err);
            closesocket(g_proxy_sock);
            g_proxy_sock = INVALID_SOCKET;
            if (!ConnectToProxy())
            {
                Log("[Portal] OpenUSB: reconnect failed - is portal_proxy.py running?");
                return false;
            }
            Log("[Portal] OpenUSB: reconnected to proxy");
        }
        g_io = new IO{};
        g_io->usb = nullptr;
        g_io->thread = std::thread(ReaderThread, g_io);
        Log("[Portal] Proxy mode: reader thread started");
        return true;
    }

    // ---- libusb mode (native Windows + WinUSB driver) ----
    if (!g_device) {
        Log("[Portal] OpenUSB: no device reference");
        return false;
    }

    libusb_device_handle* usb = nullptr;
    int r = libusb_open(g_device, &usb);
    if (r != LIBUSB_SUCCESS) {
        Log("[Portal] libusb_open failed: %s", libusb_error_name(r));
        return false;
    }

    libusb_set_auto_detach_kernel_driver(usb, 1);
    if (libusb_claim_interface(usb, IFACE) != LIBUSB_SUCCESS) {
        Log("[Portal] claim_interface failed");
        libusb_close(usb);
        return false;
    }

    libusb_control_transfer(usb,
        LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_OUT,
        0x0A, 0, IFACE, nullptr, 0, 500);
    libusb_control_transfer(usb,
        LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_OUT,
        0x0B, 1, IFACE, nullptr, 0, 500);

    g_io = new IO{};
    g_io->usb = usb;
    g_io->thread = std::thread(ReaderThread, g_io);
    Log("[Portal] USB opened, reader thread started");
    return true;
}

// close the libusb device and stop the reader thread
// called when portal.dll closes the REAL handle
inline void CloseUSB()
{
    if (!g_io) return;

    // in proxy mode, keep the IO and reader thread alive across all the open / close cycles portal.dll does for capability scanning
    // ArmRead will re-point the buffer when the next ReadFile arrives
    // actual teardown happens in ShutdownProxy() at DLL unload.
    if (g_proxy_sock != INVALID_SOCKET) {
        Log("[Portal] CloseUSB: proxy mode - keeping reader thread alive");
        return;
    }

    // libusb mode: full teardown
    {
        std::lock_guard<std::mutex> lk(g_io->mtx);
        g_io->stop = true;
    }
    g_io->cv.notify_all();

    // close the USB handle to unblock any in-flight libusb transfer
    if (g_io->usb) {
        libusb_release_interface(g_io->usb, IFACE);
        libusb_close(g_io->usb);
        g_io->usb = nullptr;
    }

    if (g_io->thread.joinable()) g_io->thread.join();

    delete g_io;
    g_io = nullptr;
    Log("[Portal] USB closed, reader thread joined");
}

inline void ShutdownProxy()
{
    if (!g_io) return;
    Log("[Portal] ShutdownProxy: stopping reader thread");
    {
        std::lock_guard<std::mutex> lk(g_io->mtx);
        g_io->stop = true;
    }
    g_io->cv.notify_all();
    if (g_io->thread.joinable()) g_io->thread.join();
    delete g_io;
    g_io = nullptr;

    if (g_proxy_sock != INVALID_SOCKET) {
        closesocket(g_proxy_sock);
        g_proxy_sock = INVALID_SOCKET;
    }
    Log("[Portal] Proxy shutdown complete");
}

// ---------------------------------------------------------------------------
// I/O operations (called from hooks)
// ---------------------------------------------------------------------------

// - called from hook_ReadFile when portal.dll arms a read on the REAL handle
// - set Internal=STATUS_PENDING so GetOverlappedResult knows the op is live, then notify the reader thread to start the libusb transfer
inline void ArmRead(BYTE* buf, LPOVERLAPPED ovlp, HANDLE owner)
{
    if (!g_io) return;

    // mark the overlapped operation as pending
    if (ovlp) {
        ovlp->Internal = static_cast<ULONG_PTR>(STATUS_PENDING_VAL);
        ovlp->InternalHigh = 0;
    }

    {
        std::lock_guard<std::mutex> lk(g_io->mtx);
        g_io->buf = buf;
        g_io->ovlp = ovlp;
        g_io->arm = true;
        g_io->owner = owner;
    }
    g_io->cv.notify_one();
}

// called from hook_HidD_SetOutputReport for fake handles
// buf[0] = report ID (0x00), buf[1..33] = command, len = 34
inline bool SendCommand(const BYTE* buf, ULONG len)
{
    if (len < 2) return false;

    // ---- proxy path ----
    if (g_proxy_sock != INVALID_SOCKET) {
        // packet: 0x01 + 32 bytes (buf+1, strip report ID, pad to 32)
        uint8_t pkt[33] = { 0x01 };
        ULONG data_len = std::min(len - 1, static_cast<ULONG>(32));
        memcpy(pkt + 1, buf + 1, data_len);
        int r = send(g_proxy_sock, reinterpret_cast<char*>(pkt), 33, 0);
        if (r != 33) {
            Log("[Portal] Proxy send failed: %d (err=%d)", r, WSAGetLastError());
            return false;
        }
        return true;
    }

    // ---- libusb path ----
    if (!g_io || !g_io->usb) {
        Log("[Portal] SendCommand: no device open");
        return false;
    }
    const int r = libusb_control_transfer(
        g_io->usb,
        LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_OUT,
        0x09,
        static_cast<uint16_t>((0x02 << 8) | buf[0]),
        IFACE,
        const_cast<uint8_t*>(buf + 1),
        static_cast<uint16_t>(std::min(static_cast<ULONG>(32), len - 1)),
        1000);
    if (r < 0) {
        Log("[Portal] SendCommand failed: %s (cmd=0x%02X)", libusb_error_name(r), buf[1]);
        return false;
    }
    return true;
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

} // namespace ssa::Portal