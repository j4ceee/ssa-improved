#pragma once
#include <Windows.h>

namespace ssa::Portal
{
    /**
     * IPortalBackend
     *
     * Interface implemented by each portal access method:
     * - <b>WinUSBBackend</b> - libusb / WinUSB driver (Windows)
     * - <b>ProxyBackend</b> - TCP proxy to native daemon (Wine / Linux)
     * - <b>HIDBackend</b> - Windows HID class driver
     * - <b>EmulatedBackend</b> - full software emulation TODO
     *
     * portal_device.h owns exactly one backend instance selected at startup and routes all portal I/O through it
     *
     * hooks never speak to a backend directly, they call the thin wrappers in portal_device.h
     */
    class IPortalBackend
    {
    public:
        virtual ~IPortalBackend() = default;

        /**
         * @return short name used in log messages (e.g. "WinUSB", "Proxy", "HID")
         */
        [[nodiscard]] virtual const char* Name() const = 0;

        /**
         * Check if the backend is available for use right now (e.g. device present, proxy reachable)
         * - called once during Init() for backend selection
         * @return true if this backend can serve as the portal right now, false otherwise
         */
        virtual bool IsAvailable() = 0;

        /**
         * Open the device / connection and start any internal threads
         * - called from the shim when portal.dll opens the overlapped (REAL) handle
         * @return true on success, false on failure
         */
        virtual bool Open() = 0;

        /**
         * Close the device / connection and stop internal threads
         * - called from the shim when portal.dll closes the REAL handle, unless PersistAcrossClose() returns true
         */
        virtual void Close() = 0;

        /**
         * Game → portal
         * @param buf Command buffer: buf[0] = HID report ID (0x00), buf[1+] = 32-byte portal command
         */
        virtual bool SendCommand(const BYTE* buf, size_t len) = 0;

        /**
         * Portal → game
         * @return bytes written (33: buf[0]=0x00, buf[1..32]=portal data) / 0 on timeout / -1 on error
         */
        virtual int ReadResponse(BYTE* buf, size_t buf_len, DWORD timeout_ms) = 0;

        /**
         * Non-blocking fast path: return a pre-buffered packet immediately if one is available, otherwise return 0
         * - used by ArmRead() to satisfy a read synchronously before the reader thread wakes (necessary for Wine where portal.dll polls WaitForSingleObject(event, 0) right after ReadFile)
         * @return bytes written (33: buf[0]=0x00, buf[1..32]=portal data) / 0 if no packet is available or this backend doesn't support pre-buffering
         */
        virtual int TryReadImmediate(BYTE* /*buf*/, size_t /*buf_len*/) { return 0; }

        /**
         * Check if the backend should survive portal.dll's capability-scan close / reopen cycles
         * (Proxy mode needs this to avoid re-connecting the TCP socket on every cycle)
         * @return <code>true</code> if the backend shouldn't be torn down
         */
        [[nodiscard]] virtual bool PersistAcrossClose() const { return false; }
    };
} // namespace ssa::Portal
