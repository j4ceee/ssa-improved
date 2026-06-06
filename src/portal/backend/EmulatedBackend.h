// Portions of this file are derived from Cemu (https://github.com/cemu-project/Cemu)
// Licensed under Mozilla Public License 2.0 - see THIRD_PARTY_NOTICES.md
#pragma once
#include "../IPortalBackend.h"
#include "../../skylanders/SkylanderManager.h"
#include "log.h"

#include <array>
#include <queue>
#include <mutex>
#include <thread>
#include <chrono>
#include <filesystem>
#include <cstring>

namespace ssa::Portal::Backend
{
    /**
     * EmulatedBackend
     *
     * Full software emulation of a Skylanders Portal of Power (SSA/Giants variant, 4-slot)
     *
     * Replaces the physical USB device entirely when g_config.emulatedPortal is true
     * All portal I/O is handled in software, no USB device is required
     *
     * Protocol behaviour mirrors RPCS3 / Cemu's emulated portal implementations:
     *  - SendCommand() dispatches the command, builds a response, and pushes it to m_queries
     *  - ReadResponse() sleeps ~20 ms (portal interrupt interval), then pops a queued response if one exists, or synthesises a status ('S') packet
     *
     * Concurrency:
     *  - SendCommand() is called from the main game thread (via hook_HidD_SetOutputReport)
     *  - ReadResponse() is called from the reader thread (portal_device.h:ReaderThread)
     *  - m_queryMutex guards the response queue
     *  - m_skyMutex  guards slot state, LED colour, and the interrupt counter
     *  - both mutexes are never held simultaneously, so there is no deadlock risk.
     *
     * Buffer layout:
     *  buf[0] = 0x00 (HID report ID, always present)
     *  buf[1] = cmd (command character: 'A', 'C', 'Q', 'W', ...)
     *  buf[2..] = args (command-specific bytes)
     */
    class EmulatedBackend final : public IPortalBackend
    {
    public:
        // -----------------------------------------------------------------------
        // Portal constants
        // -----------------------------------------------------------------------
        static constexpr int SLOT_COUNT = 4;
        static constexpr int DATA_SIZE = 1024; // MIFARE Classic 1K
        static constexpr int PACKET_SIZE = 32; // raw portal data (no HID prefix)
        static constexpr DWORD INTERVAL_MS = 20; // ~50 Hz interrupt rate

        // per-slot status values (2 bits, matching the physical portal protocol)
        static constexpr uint8_t ST_ABSENT = 0;
        static constexpr uint8_t ST_PRESENT = 1;
        static constexpr uint8_t ST_REMOVING = 2;
        static constexpr uint8_t ST_ADDED = 3;

        // -----------------------------------------------------------------------
        // Slot state (also exposed to the UI for display)
        // -----------------------------------------------------------------------
        struct SlotDisplay
        {
            bool occupied;
            std::filesystem::path filePath; // empty if unoccupied & or temporary figures
            std::string displayName; // non-empty for temporary figures
        };

        // -----------------------------------------------------------------------
        // IPortalBackend interface
        // -----------------------------------------------------------------------
        [[nodiscard]] const char* Name() const override { return "Emulated"; }

        bool PersistAcrossClose() const override
        {
            static const bool isWine = []()
            {
                HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
                return ntdll && GetProcAddress(ntdll, "wine_get_version") != nullptr;
            }();
            return isWine; // on Wine persist, elsewhere don't
        }

        bool IsAvailable() override { return true; }

        bool Open() override
        {
            Log("[Emulated] Portal opened");
            return true;
        }

        void Close() override
        {
            Log("[Emulated] Portal closed");
        }

        /**
         * Dispatches an incoming portal command (host → portal)
         *
         * buf[0] = HID report ID (0x00, always ignored here)
         * buf[1] = command character
         * buf[2..] = command arguments
         */
        bool SendCommand(const BYTE* buf, size_t len) override
        {
            if (!buf || len < 2) return false;

            const uint8_t cmd = buf[1]; // buf[0] is the HID report ID
            std::array<uint8_t, PACKET_SIZE> response{};
            bool hasResponse = false;

            switch (cmd)
            {
            case 'A': // Activate
                {
                    // response: ['A', echo_byte, 0xFF, 0x77]
                    response[0] = 0x41;
                    response[1] = (len > 2) ? buf[2] : 0x01;
                    response[2] = 0xFF;
                    response[3] = 0x77;
                    hasResponse = true;
                    Activate();
                    break;
                }
            case 'C': // Set colour (all sides)
                {
                    // buf[2]=R, buf[3]=G, buf[4]=B
                    if (len >= 5)
                        SetLeds(0x01, buf[2], buf[3], buf[4]);
                    break; // no immediate response
                }
            case 'J': // Set colour alternative (Trap Team side lights, 7-byte command)
                {
                    // buf[2]=side, buf[3]=R, buf[4]=G, buf[5]=B, buf[6..7]=fade duration (ignored)
                    if (len >= 6)
                        SetLeds(buf[2], buf[3], buf[4], buf[5]);
                    response[0] = 0x4A;
                    hasResponse = true;
                    break;
                }
            case 'L': // Side lights (Trap Team portal, 5-byte command)
                {
                    // buf[2]=side (0x00=right, 0x02=left, 0x01=both, 0x03=trap light)
                    // buf[3]=R, buf[4]=G, buf[5]=B
                    if (len >= 6)
                    {
                        uint8_t side = buf[2];
                        if (side == 0x02) side = 0x04; // Cemu remapping
                        SetLeds(side, buf[3], buf[4], buf[5]);
                    }
                    break; // no response
                }
            case 'M': // Audio firmware version query
                {
                    // return version 0 to suppress portal audio attempts
                    response[0] = 0x4D;
                    response[1] = (len > 2) ? buf[2] : 0x00;
                    response[2] = 0x00;
                    response[3] = 0x19;
                    hasResponse = true;
                    break;
                }
            case 'Q': // query NFC block
                {
                    // buf[2]=slot_byte (slot = buf[2] & 0xF), buf[3]=block_index
                    if (len >= 4)
                    {
                        const uint8_t skyNum = buf[2] & 0xF;
                        const uint8_t block = buf[3];
                        if (block < 0x40)
                        {
                            QueryBlock(skyNum, block, response.data());
                            hasResponse = true;
                        }
                    }
                    break;
                }
            case 'R': // Reset / deactivate
                {
                    // response: ['R', 0x02, 0x1b]
                    response[0] = 0x52;
                    response[1] = 0x02;
                    response[2] = 0x1B;
                    hasResponse = true;
                    Deactivate();
                    break;
                }
            case 'S': // Status query (game just polling; portal responds autonomously)
            case 'V': // Unknown, no response needed
                break;
            case 'W': // Write NFC block
                {
                    // buf[2]=slot_byte, buf[3]=block_index, buf[4..19]=16 bytes of data
                    if (len >= 20)
                    {
                        const uint8_t skyNum = buf[2] & 0xF;
                        const uint8_t block = buf[3];
                        if (block < 0x40)
                        {
                            WriteBlock(skyNum, block, buf + 4, response.data());
                            hasResponse = true;
                        }
                    }
                    break;
                }
            default:
                Log("[Emulated] Unknown command: 0x%02X ('%c')", cmd, (cmd >= 0x20 && cmd < 0x7F) ? cmd : '?');
                break;
            }

            if (hasResponse)
            {
                std::lock_guard lk(m_queryMutex);
                m_queries.push(response);
            }
            return true;
        }

        /**
         * Returns the next packet to deliver to the game
         *
         * Sleeps INTERVAL_MS to simulate the portal's interrupt rate, then either pops a queued command response or synthesises a status packet
         *
         * buf[0]    = 0x00 (HID report ID prefix expected by the shim)
         * buf[1..32] = 32 bytes of portal data
         * @return 33 on success, -1 if buf_len is too small
         */
        int ReadResponse(BYTE* buf, size_t buf_len, DWORD /*timeout_ms*/) override
        {
            if (!buf || buf_len < 33) return -1;

            std::this_thread::sleep_for(std::chrono::milliseconds(INTERVAL_MS));

            buf[0] = 0x00; // HID report ID

            {
                std::lock_guard lk(m_queryMutex);
                if (!m_queries.empty())
                {
                    memcpy(buf + 1, m_queries.front().data(), PACKET_SIZE);
                    m_queries.pop();
                    return 33;
                }
            }

            // no queued response → emit status ('S') packet
            BuildStatusPacket(buf + 1);
            return 33;
        }

        // -----------------------------------------------------------------------
        // UI-facing methods  (called from portal_tab.cpp)
        // -----------------------------------------------------------------------

        /**
         * Load a skylander dump into a specific UI slot (0-3)
         *
         * Reads the file, stores data in-memory, and queues the ADDED→PRESENT status transition so the game detects the figure immediately
         */
        bool LoadSkylander(int slot, const std::filesystem::path& filePath)
        {
            if (slot < 0 || slot >= SLOT_COUNT) return false;

            std::array<uint8_t, DATA_SIZE> data{};
            if (!Skylanders::SkylanderManager::ReadFigureData(filePath, data.data()))
            {
                Log("[Emulated] LoadSkylander: failed to read %ls", filePath.c_str());
                return false;
            }

            const uint32_t serial = static_cast<uint32_t>(data[0])
                | static_cast<uint32_t>(data[1]) << 8
                | static_cast<uint32_t>(data[2]) << 16
                | static_cast<uint32_t>(data[3]) << 24;

            std::lock_guard lk(m_skyMutex);
            auto& s = m_slots[slot];

            // drain any pending status transitions from the previous figure
            s.queuedStatus = std::queue<uint8_t>();

            s.data = data;
            s.filePath = filePath;
            s.displayName.clear(); // library-backed; UI resolves the label via the library
            s.lastId = serial;
            s.status = ST_ADDED;
            s.queuedStatus.push(ST_ADDED);
            s.queuedStatus.push(ST_PRESENT);

            Log("[Emulated] Slot %d loaded: %ls (serial=0x%08X)", slot, filePath.c_str(), serial);
            return true;
        }

        /**
         * Generate figure data in memory for a temporary figure (item, mini, adventure pack) and load it into a specific UI slot (0-3)
         *
         * No file is created or read. Writes from the game are buffered in-memory and discarded on removal.
         */
        bool LoadTemporaryFigure(int slot, uint16_t figureId, uint16_t variantId, const char* displayName)
        {
            if (slot < 0 || slot >= SLOT_COUNT) return false;

            std::array<uint8_t, DATA_SIZE> data{};
            Skylanders::SkylanderManager::GenerateFigureData(figureId, variantId, data.data());

            const uint32_t serial = static_cast<uint32_t>(data[0])
                | static_cast<uint32_t>(data[1]) << 8
                | static_cast<uint32_t>(data[2]) << 16
                | static_cast<uint32_t>(data[3]) << 24;

            std::lock_guard lk(m_skyMutex);
            auto& s = m_slots[slot];

            // drain any pending status transitions from the previous figure
            s.queuedStatus = std::queue<uint8_t>();

            s.data = data;
            s.filePath.clear(); // no backing file, WriteBlock will skip disk flush automatically
            s.displayName = displayName ? displayName : "";
            s.lastId = serial;
            s.status = ST_ADDED;
            s.queuedStatus.push(ST_ADDED);
            s.queuedStatus.push(ST_PRESENT);

            Log("[Emulated] Slot %d loaded ephemeral: %s (ID: %04X, Variant: %04X, serial=0x%08X)",
                slot, displayName, figureId, variantId, serial);
            return true;
        }

        /**
         * Remove the skylander from a UI slot (0-3)
         *
         * Queues the REMOVING→ABSENT transition
         */
        bool RemoveSkylander(int slot)
        {
            if (slot < 0 || slot >= SLOT_COUNT) return false;

            std::lock_guard lk(m_skyMutex);
            auto& s = m_slots[slot];

            if (!(s.status & 1)) return false; // already absent

            s.status = ST_REMOVING;
            s.queuedStatus.push(ST_REMOVING);
            s.queuedStatus.push(ST_ABSENT);
            s.filePath.clear();
            s.displayName.clear();

            Log("[Emulated] Slot %d removed", slot);
            return true;
        }

        /**
         * Lightweight snapshot of a slot's display state.
         * Safe to call from any thread (takes m_skyMutex briefly).
         */
        SlotDisplay GetSlotDisplay(int slot) const
        {
            if (slot < 0 || slot >= SLOT_COUNT)
                return {false, {}};
            std::lock_guard lk(m_skyMutex);
            const auto& s = m_slots[slot];
            return {(s.status & 1) != 0, s.filePath, s.displayName};
        }

        /**
         * Current portal LED colour
         */
        struct Color
        {
            uint8_t r = 0, g = 0, b = 0;
        };

        Color GetPortalColor() const
        {
            std::lock_guard lk(m_skyMutex);
            return m_colorRight;
        }

    private:
        // -----------------------------------------------------------------------
        // Internal slot state
        // -----------------------------------------------------------------------
        struct Slot
        {
            std::array<uint8_t, DATA_SIZE> data{};
            std::filesystem::path filePath;
            std::string displayName; // non-empty for temp figures, empty for library-backed
            uint32_t lastId = 0;
            uint8_t status = ST_ABSENT;
            std::queue<uint8_t> queuedStatus;
        };

        std::array<Slot, SLOT_COUNT> m_slots;
        mutable std::mutex m_skyMutex; // guards slots, LEDs, counter

        std::queue<std::array<uint8_t, PACKET_SIZE>> m_queries;
        mutable std::mutex m_queryMutex; // guards response queue

        bool m_activated = false;
        uint8_t m_interruptCounter = 0;

        Color m_colorRight;
        Color m_colorLeft;
        Color m_colorTrap;

        // -----------------------------------------------------------------------
        // Portal state transitions
        // -----------------------------------------------------------------------

        void Activate()
        {
            std::lock_guard lk(m_skyMutex);
            if (m_activated) return;

            // re-announce every figure already on the portal
            for (auto& s : m_slots)
            {
                if (s.status & 1)
                {
                    s.queuedStatus.push(ST_ADDED);
                    s.queuedStatus.push(ST_PRESENT);
                }
            }
            m_activated = true;
            Log("[Emulated] Activated");
        }

        void Deactivate()
        {
            std::lock_guard lk(m_skyMutex);
            for (auto& s : m_slots)
            {
                // drain the queue and keep the last known stable status
                if (!s.queuedStatus.empty())
                {
                    s.status = s.queuedStatus.back();
                    s.queuedStatus = std::queue<uint8_t>();
                }
                s.status &= ST_PRESENT; // preserve the "is a figure here?" bit
            }
            m_activated = false;
            Log("[Emulated] Deactivated");
        }

        /**
         * side: 0x00 = right, 0x01 = both, 0x02 / 0x04 = left, 0x03 = trap light
         */
        void SetLeds(uint8_t side, uint8_t r, uint8_t g, uint8_t b)
        {
            std::lock_guard lk(m_skyMutex);
            if (side == 0x00)
            {
                m_colorRight = {r, g, b};
            }
            else if (side == 0x01)
            {
                m_colorRight = {r, g, b};
                m_colorLeft = {r, g, b};
            }
            else if (side == 0x02 || side == 0x04)
            {
                m_colorLeft = {r, g, b};
            }
            else if (side == 0x03)
            {
                m_colorTrap = {r, g, b};
            }
        }

        // -----------------------------------------------------------------------
        // NFC block access
        // -----------------------------------------------------------------------

        void QueryBlock(uint8_t skyNum, uint8_t block, uint8_t* replyBuf)
        {
            std::lock_guard lk(m_skyMutex);

            replyBuf[0] = 'Q';
            replyBuf[2] = block;

            if (skyNum >= SLOT_COUNT || !(m_slots[skyNum].status & 1))
            {
                replyBuf[1] = skyNum; // figure not present
                return;
            }

            replyBuf[1] = 0x10 | skyNum; // 0x10 flag = figure present
            memcpy(replyBuf + 3, m_slots[skyNum].data.data() + (16 * block), 16);
        }

        void WriteBlock(uint8_t skyNum, uint8_t block, const uint8_t* toWrite, uint8_t* replyBuf)
        {
            std::lock_guard lk(m_skyMutex);

            replyBuf[0] = 'W';
            replyBuf[2] = block;

            if (skyNum >= SLOT_COUNT || !(m_slots[skyNum].status & 1))
            {
                replyBuf[1] = skyNum;
                return;
            }

            auto& s = m_slots[skyNum];
            replyBuf[1] = 0x10 | skyNum;
            memcpy(s.data.data() + (block * 16), toWrite, 16);

            // persist to disk immediately (1 KB write, should be negligible)
            // temp figures have an empty filePath, so this is a no-op for them
            if (!s.filePath.empty())
                Skylanders::SkylanderManager::SaveFigureData(s.filePath, s.data.data());
        }

        // -----------------------------------------------------------------------
        // Status packet builder
        // -----------------------------------------------------------------------

        /**
         * Writes a 32-byte 'S' (status) packet into buf[0..31]
         * Status encoding: 2 bits per slot, packed little-endian -> 4 slots, fits in a single byte at buf[1]
         */
        void BuildStatusPacket(uint8_t* buf)
        {
            std::lock_guard lk(m_skyMutex);

            uint8_t statusByte = 0;
            for (int i = SLOT_COUNT - 1; i >= 0; i--)
            {
                auto& s = m_slots[i];
                if (!s.queuedStatus.empty())
                {
                    s.status = s.queuedStatus.front();
                    s.queuedStatus.pop();
                }
                statusByte <<= 2;
                statusByte |= (s.status & 0x3);
            }

            memset(buf, 0, PACKET_SIZE);
            buf[0] = 0x53; // 'S'
            buf[1] = statusByte;
            buf[5] = m_interruptCounter++;
            buf[6] = m_activated ? 0x01 : 0x00;
        }
    };

    // ---------------------------------------------------------------------------
    // Public colour accessor
    // ---------------------------------------------------------------------------
    using EmulatedColor = EmulatedBackend::Color;
} // namespace ssa::Portal::Backend
