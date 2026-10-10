#pragma once
#include <Windows.h>
#include <atomic>
#include <cstdint>
#include <cstring>
#include "addresses.h"
#include "log.h"

// Replaces the game's shader constant lookup -> SecuROM slows down the original function
// - binary search in the shader's constant table (8 byte entries, sorted by an unsigned CRC32 at offset 0), returning the matching entry or the end pointer
// - in PC executable SecuROM moved its body into the "Stext" section, obfuscated it, and left a stub in .text: 0x004297B0:  JMP DWORD PTR [0x01DB9218]
// - slot at 0x01DB9218 is plain data (in the .securom section): this points it at an equivalent function of the mod
namespace ssa::Game::ConstantLookup
{
    // CONSTANT_LOOKUP_THUNK is the stub in .text
    inline uintptr_t Thunk() { return reinterpret_cast<uintptr_t>(GetAddress(CONSTANT_LOOKUP_THUNK)); }
    inline uintptr_t Slot() { return reinterpret_cast<uintptr_t>(GetAddress(CONSTANT_LOOKUP_SLOT)); }

    inline bool g_enabled = true;
    // on by default, can be switched off in Diagnostics (it replaces a function by changing a pointer in SecuROM's data)
    inline bool g_verify = false; // also run the game's own function and compare
    inline bool g_available = false; // the thunk and the slot look as expected
    inline void* g_original = nullptr; // what the slot pointed to before we changed it
    inline std::atomic<uint32_t> g_verified{0};
    inline std::atomic<uint32_t> g_mismatches{0};

    // same search as the game's (unsigned CRC32 keys, 8 byte entries): the entry with the key, or `end`
    inline const uint32_t* __cdecl Find(const uint32_t* begin, const uint32_t* end, const uint32_t key)
    {
        if (end <= begin)
            return end;

        size_t lo = 0;
        size_t hi = static_cast<size_t>(end - begin) / 2; // entries
        while (lo < hi)
        {
            const size_t mid = lo + (hi - lo) / 2;
            const uint32_t k = begin[mid * 2];
            if (k < key)
                lo = mid + 1;
            else if (k > key)
                hi = mid;
            else
                return begin + mid * 2;
        }
        return end;
    }

    // the game's own function (through the pointer that was in the slot), same convention as the call sites use
    inline const uint32_t* CallOriginal(const uint32_t* first, const uint32_t* last, const uint32_t key)
    {
        void* fn = g_original;
        const uint32_t* result = nullptr;
        __asm {
            mov ecx, first
            mov eax, last
            push key
            call fn
            add esp, 4
            mov result, eax
            }
        return result;
    }

    // called from the stub (plain cdecl)
    inline const uint32_t* __cdecl Lookup(const uint32_t* begin, const uint32_t* end, const uint32_t key)
    {
        const uint32_t* mine = Find(begin, end, key);

        if (g_verify && g_original)
        {
            const uint32_t* theirs = CallOriginal(begin, end, key);
            ++g_verified;
            if (theirs != mine)
            {
                if (g_mismatches.fetch_add(1) < 8)
                    Log("[ConstantLookup] MISMATCH key=0x%08X begin=%p end=%p mine=%p game=%p", key, begin, end, mine,
                        theirs);
                return theirs; // the game's answer always wins
            }
        }
        return mine;
    }

    // entry point for the 15 call sites: ECX = begin, EAX = end, [ESP + 4] = key, returns in EAX.
    inline __declspec(naked) void Stub()
    {
        __asm {
            push ecx
            push edx
            push dword ptr [esp + 12] // key: [esp + 4] on entry, two pushes later
            push eax // end
            push ecx // begin
            call Lookup
            add esp, 12
            pop edx
            pop ecx
            ret
            }
    }

    // is the executable what we expect: thunk jumps through the slot & slot is writable memory holding a pointer
    inline bool Verify()
    {
        const auto* thunk = reinterpret_cast<const uint8_t*>(Thunk());
        MEMORY_BASIC_INFORMATION mbi = {};
        if (!VirtualQuery(thunk, &mbi, sizeof(mbi)) || mbi.State != MEM_COMMIT)
        {
            Log("[ConstantLookup] unavailable: thunk address is not mapped");
            return false;
        }

        uint32_t target = 0;
        std::memcpy(&target, thunk + 2, sizeof(target));
        if (thunk[0] != 0xFF || thunk[1] != 0x25 || target != Slot())
        {
            Log("[ConstantLookup] unavailable: 0x%08X is not JMP [0x%08X] (bytes %02X %02X ...)",
                static_cast<unsigned>(Thunk()),
                static_cast<unsigned>(Slot()), thunk[0], thunk[1]);
            return false;
        }

        if (!VirtualQuery(reinterpret_cast<const void*>(Slot()), &mbi, sizeof(mbi)) || mbi.State != MEM_COMMIT ||
            !(mbi.Protect & (PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)))
        {
            Log("[ConstantLookup] unavailable: the slot 0x%08X is not writable", static_cast<unsigned>(Slot()));
            return false;
        }

        if (!*reinterpret_cast<void* const*>(Slot()))
        {
            Log("[ConstantLookup] unavailable: the slot is empty");
            return false;
        }

        Log("[ConstantLookup] thunk and slot verified, slot currently points to %p",
            *reinterpret_cast<void* const*>(Slot()));
        return true;
    }

    // called every frame from Present: keeps the slot in the state the switch asks for
    inline void Update()
    {
        static bool s_checked = false;
        if (!s_checked)
        {
            s_checked = true;
            g_available = Verify();
        }
        if (!g_available)
            return;

        void** slot = reinterpret_cast<void**>(Slot());
        void* stub = reinterpret_cast<void*>(&Stub);

        if (g_enabled)
        {
            if (*slot != stub)
            {
                g_original = *slot; // whatever SecuROM has there right now
                *slot = stub;
                Log("[ConstantLookup] enabled (game function at %p)", g_original);
            }
        }
        else if (g_original && *slot == stub)
        {
            *slot = g_original;
            Log("[ConstantLookup] disabled");
        }
    }
} // namespace ssa::Game::ConstantLookup
