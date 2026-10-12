#pragma once
#include <Windows.h>
#include <atomic>
#include <cstdint>
#include <cstring>
#include "addresses.h"
#include "log.h"

// Replacing a function that SecuROM protected, without touching the game's code:
// - SecuROM moved the body into its own obfuscated code, where every call costs many times what the function does
// - the slot is plain data (in the .securom section, writable): pointing it at a function of the mod replaces the function for every
//   caller, and writing the old value back undoes it. The game's .text is never written, which SecuROM would notice.
namespace ssa::Securom
{
    // for the verify mode of a replacement: how many results were compared with the game's, and how many were different
    struct VerifyCounters
    {
        std::atomic<uint32_t> compared{0};
        std::atomic<uint32_t> mismatches{0};
    };

    class SlotPatch
    {
    public:
        // `stub` has to take the arguments the stub in .text is called with and return the way the original does.
        // `enabledAtStart`: false for a replacement that has not been verified enough to be on by default
        SlotPatch(const char* name, const Address thunk, const Address slot, void* stub,
                  const bool enabledAtStart = true)
            : enabled(enabledAtStart), m_name(name), m_thunk(thunk), m_slot(slot), m_stub(stub)
        {
        }

        SlotPatch(const SlotPatch&) = delete;
        SlotPatch& operator=(const SlotPatch&) = delete;

        bool enabled; // switchable in Diagnostics

        // thunk and the slot look as expected
        [[nodiscard]] bool available() const { return m_available; }
        // what the slot pointed to before it was replaced: the game's own function, as SecuROM has it
        [[nodiscard]] void* original() const { return m_original; }
        [[nodiscard]] uintptr_t slot() const { return reinterpret_cast<uintptr_t>(GetAddress(m_slot)); }
        [[nodiscard]] const char* name() const { return m_name; }

        // call every frame (from Present): keeps the slot in the state `enabled` asks for
        void Update()
        {
            if (!m_available)
            {
                // slot can still be empty in the first frames; a wrong thunk never gets better
                if (m_failedForGood || m_attempts >= kMaxAttempts)
                    return;
                ++m_attempts;
                m_available = Verify();
                if (!m_available)
                    return;
            }

            void** slotPtr = reinterpret_cast<void**>(slot());
            if (enabled)
            {
                if (*slotPtr != m_stub)
                {
                    m_original = *slotPtr; // whatever SecuROM has there right now
                    *slotPtr = m_stub;
                    if (m_logged++ < kMaxLogs)
                        Log("[%s] enabled (game function at %p)", m_name, m_original);
                }
            }
            else if (m_original && *slotPtr == m_stub)
            {
                *slotPtr = m_original;
                Log("[%s] disabled", m_name);
            }
        }

    private:
        static constexpr uint32_t kMaxAttempts = 600; // frames to wait for SecuROM to fill the slot
        static constexpr uint32_t kMaxLogs = 8; // SecuROM could take the slot back over and over

        // is the executable what we expect: the thunk jumps through the slot, and the slot is writable memory holding a pointer
        bool Verify()
        {
            const auto* thunk = reinterpret_cast<const uint8_t*>(GetAddress(m_thunk));
            MEMORY_BASIC_INFORMATION mbi = {};
            if (!VirtualQuery(thunk, &mbi, sizeof(mbi)) || mbi.State != MEM_COMMIT)
            {
                Log("[%s] unavailable: thunk address is not mapped", m_name);
                return Fail();
            }

            uint32_t target = 0;
            std::memcpy(&target, thunk + 2, sizeof(target));
            if (thunk[0] != 0xFF || thunk[1] != 0x25 || target != slot())
            {
                Log("[%s] unavailable: 0x%08X is not JMP [0x%08X] (bytes %02X %02X ...)", m_name,
                    static_cast<unsigned>(reinterpret_cast<uintptr_t>(thunk)), static_cast<unsigned>(slot()), thunk[0],
                    thunk[1]);
                return Fail();
            }

            if (!VirtualQuery(reinterpret_cast<const void*>(slot()), &mbi, sizeof(mbi)) || mbi.State != MEM_COMMIT ||
                !(mbi.Protect & (PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)))
            {
                Log("[%s] unavailable: the slot 0x%08X is not writable", m_name, static_cast<unsigned>(slot()));
                return Fail();
            }

            if (!*reinterpret_cast<void* const*>(slot()))
                return false; // not filled yet, try again next frame

            Log("[%s] thunk and slot verified, slot currently points to %p", m_name,
                *reinterpret_cast<void* const*>(slot()));
            return true;
        }

        bool Fail()
        {
            m_failedForGood = true;
            return false;
        }

        const char* m_name;
        Address m_thunk;
        Address m_slot;
        void* m_stub;

        void* m_original = nullptr;
        bool m_available = false;
        bool m_failedForGood = false;
        uint32_t m_attempts = 0;
        uint32_t m_logged = 0;
    };
} // namespace ssa::Securom
