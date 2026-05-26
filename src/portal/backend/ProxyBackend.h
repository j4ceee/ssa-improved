#pragma once
#include "../IPortalBackend.h"
#include "log.h"
#include <winsock2.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace ssa::Portal::Backend
{
    /**
     * ProxyBackend
     *
     * Forwards portal I/O over a local TCP connection to portal_proxy.py, a native daemon that holds the real USB device
     * (used with Wine / Proton on Linux)
     *
     * Data format:
     * - host → daemon: 0x01 + 32-byte command (33 bytes total)
     * - daemon → host: 0x02 + 32-byte response (33 bytes total)
     *
     * Always-on recv thread continuously pre-buffers the latest packet so that TryReadImmediate() can satisfy ArmRead() synchronously
     * (required because Wine's portal.dll polls WaitForSingleObject(event, 0) immediately after ReadFile with no time for the reader thread to wake up and call ReadResponse())
     *
     * PersistAcrossClose() returns true: portal.dll's capability-scan open / close cycles must not tear down and reconnect the socket each time
     *
     */
    class ProxyBackend final : public IPortalBackend
    {
    public:
        static constexpr uint16_t PORT = 8764;
        static constexpr DWORD RECV_TIMEOUT_MS = 100;

        ~ProxyBackend() override { Close(); }

        [[nodiscard]] const char* Name() const override { return "Proxy"; }
        bool PersistAcrossClose() const override { return true; }

        bool IsAvailable() override
        {
            return (sock_ != INVALID_SOCKET) || TryConnect();
        }

        bool Open() override
        {
            if (!IsAvailable())
            {
                Log("[Proxy] Not available - is portal_proxy.py running?");
                return false;
            }
            StartRecvThread();
            Log("[Proxy] Opened");
            return true;
        }

        void Close() override
        {
            stop_.store(true);
            pkt_cv_.notify_all();
            if (recv_thread_.joinable()) recv_thread_.join();

            if (sock_ != INVALID_SOCKET)
            {
                closesocket(sock_);
                sock_ = INVALID_SOCKET;
            }
            {
                std::lock_guard lk(pkt_mtx_);
                pkt_ready_ = false;
            }
            Log("[Proxy] Closed");
        }

        bool SendCommand(const BYTE* buf, size_t len) override
        {
            if (sock_ == INVALID_SOCKET || len < 2) return false;

            uint8_t pkt[33] = {0x01};
            memcpy(pkt + 1, buf + 1, std::min<size_t>(len - 1, 32));

            int r = send(sock_, reinterpret_cast<char*>(pkt), 33, 0);
            if (r != 33)
            {
                Log("[Proxy] send failed: %d (err=%d)", r, WSAGetLastError());
                return false;
            }
            return true;
        }

        // blocking: waits up to timeout_ms for the recv thread to buffer a packet
        int ReadResponse(BYTE* buf, size_t buf_len, DWORD timeout_ms) override
        {
            if (buf_len < 33) return -1;

            std::unique_lock lk(pkt_mtx_);
            bool ok = pkt_cv_.wait_for(lk, std::chrono::milliseconds(timeout_ms),
                                       [this] { return pkt_ready_ || stop_.load(); });
            if (!ok) return 0; // timeout
            if (stop_.load()) return -1;
            return DeliverLocked(buf);
        }

        // non-blocking: if a packet is already sitting in pkt_buf_, deliver it now
        int TryReadImmediate(BYTE* buf, size_t buf_len) override
        {
            if (buf_len < 33) return 0;
            std::lock_guard lk(pkt_mtx_);
            if (!pkt_ready_) return 0;
            return DeliverLocked(buf);
        }

    private:
        // ---------------------------------------------------------------------------
        // Socket helpers
        // ---------------------------------------------------------------------------

        bool TryConnect()
        {
            WSADATA wsa;
            if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return false;

            SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
            if (s == INVALID_SOCKET) return false;

            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port = htons(PORT);
            addr.sin_addr.s_addr = inet_addr("127.0.0.1");

            if (connect(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0)
            {
                closesocket(s);
                return false;
            }
            sock_ = s;
            return true;
        }

        // ---------------------------------------------------------------------------
        // Always-on recv thread
        // ---------------------------------------------------------------------------

        void StartRecvThread()
        {
            stop_.store(false);
            recv_thread_ = std::thread([this] { RecvLoop(); });
        }

        void RecvLoop()
        {
            Log("[Proxy] Recv thread started");

            DWORD t = RECV_TIMEOUT_MS;
            setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char*>(&t), sizeof(t));

            while (!stop_.load())
            {
                uint8_t pkt[33] = {};
                int received = 0;

                // accumulate a full 33-byte frame
                while (received < 33 && !stop_.load())
                {
                    int r = recv(sock_,
                                 reinterpret_cast<char*>(pkt) + received,
                                 33 - received, 0);
                    if (r > 0)
                    {
                        received += r;
                        continue;
                    }

                    int err = WSAGetLastError();
                    if (err == WSAETIMEDOUT || err == WSAEWOULDBLOCK) break;
                    if (!stop_.load()) Log("[Proxy] recv error: %d", err);
                    goto done;
                }

                if (received < 33 || pkt[0] != 0x02 || stop_.load()) continue;

                {
                    std::lock_guard lk(pkt_mtx_);
                    memcpy(pkt_buf_, pkt, 33);
                    pkt_ready_ = true;
                }
                pkt_cv_.notify_all();
            }

        done:
            Log("[Proxy] Recv thread exiting");
        }

        // ---------------------------------------------------------------------------
        // Delivery helper - caller must hold pkt_mtx_
        // ---------------------------------------------------------------------------

        int DeliverLocked(BYTE* buf)
        {
            buf[0] = 0x00; // report ID prefix
            memcpy(buf + 1, pkt_buf_ + 1, 32);
            pkt_ready_ = false;
            return 33;
        }

        SOCKET sock_ = INVALID_SOCKET;
        std::thread recv_thread_;
        std::mutex pkt_mtx_;
        std::condition_variable pkt_cv_;
        uint8_t pkt_buf_[33] = {};
        bool pkt_ready_ = false;
        std::atomic<bool> stop_{false};
    };
} // namespace ssa::Portal::Backend
