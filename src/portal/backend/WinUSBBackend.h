#pragma once
#include "../IPortalBackend.h"
#include "libusb.h"
#include "log.h"
#include <algorithm>

namespace ssa::Portal::Backend
{
    /**
     * WinUSBBackend
     *
     * Communicates with a portal that has the WinUSB driver installed instead of the default HID driver.
     * Wraps libusb for both commands (control transfer on EP0) and responses (interrupt transfer on EP_IN 0x81).
     */
    class WinUSBBackend final : public IPortalBackend
    {
    public:
        static constexpr uint16_t VID = 0x1430;
        static constexpr uint16_t PID = 0x0150;
        static constexpr uint8_t EP_IN = 0x81;
        static constexpr int IFACE = 0;

        // takes ownership of ctx and a ref-counted device reference (both are released in the destructor)
        explicit WinUSBBackend(libusb_context* ctx, libusb_device* device) : ctx_(ctx), device_(device)
        {

        }

        ~WinUSBBackend() override
        {
            Close();
            if (device_)
            {
                libusb_unref_device(device_);
                device_ = nullptr;
            }
            if (ctx_)
            {
                libusb_exit(ctx_);
                ctx_ = nullptr;
            }
        }

        [[nodiscard]] const char* Name() const override { return "WinUSB"; }
        bool IsAvailable() override { return device_ != nullptr; }

        bool Open() override
        {
            if (usb_) return true; // already open

            if (libusb_open(device_, &usb_) != LIBUSB_SUCCESS)
            {
                Log("[WinUSB] libusb_open failed");
                return false;
            }
            libusb_set_auto_detach_kernel_driver(usb_, 1);
            if (libusb_claim_interface(usb_, IFACE) != LIBUSB_SUCCESS)
            {
                Log("[WinUSB] claim_interface failed");
                libusb_close(usb_);
                usb_ = nullptr;
                return false;
            }

            // set-Protocol (boot) + set-Idle
            libusb_control_transfer(usb_,
                                    LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_OUT,
                                    0x0A, 0, IFACE, nullptr, 0, 500);
            libusb_control_transfer(usb_,
                                    LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_OUT,
                                    0x0B, 1, IFACE, nullptr, 0, 500);

            Log("[WinUSB] Opened");
            return true;
        }

        void Close() override
        {
            if (!usb_) return;
            libusb_release_interface(usb_, IFACE);
            libusb_close(usb_);
            usb_ = nullptr;
            Log("[WinUSB] Closed");
        }

        bool SendCommand(const BYTE* buf, size_t len) override
        {
            if (!usb_ || len < 2) return false;

            // HID SET_REPORT (class request 0x09) on EP0
            int r = libusb_control_transfer(
                usb_,
                LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_OUT,
                0x09,
                static_cast<uint16_t>((0x02 << 8) | buf[0]), // wValue: ReportType | ReportID
                IFACE,
                const_cast<uint8_t*>(buf + 1),
                static_cast<uint16_t>(std::min<size_t>(32, len - 1)),
                1000);

            if (r < 0)
            {
                Log("[WinUSB] SendCommand failed: %s (cmd=0x%02X)", libusb_error_name(r), buf[1]);
                return false;
            }
            return true;
        }

        // blocking interrupt IN transfer, returns 33 on success, 0 on timeout, -1 on error.
        int ReadResponse(BYTE* buf, size_t buf_len, DWORD timeout_ms) override
        {
            if (!usb_ || buf_len < 33) return -1;

            uint8_t raw[32] = {};
            int got = 0;
            int r = libusb_interrupt_transfer(usb_, EP_IN, raw, sizeof(raw), &got, timeout_ms);

            if (r == LIBUSB_SUCCESS && got > 0)
            {
                buf[0] = 0x00; // report ID prefix expected by the shim
                memcpy(buf + 1, raw, static_cast<size_t>(got));
                return 1 + got;
            }
            if (r == LIBUSB_ERROR_TIMEOUT) return 0;

            Log("[WinUSB] Transfer error: %s", libusb_error_name(r));
            return -1;
        }

    private:
        libusb_context* ctx_ = nullptr;
        libusb_device* device_ = nullptr;
        libusb_device_handle* usb_ = nullptr;
    };
} // namespace ssa::Portal::Backend
