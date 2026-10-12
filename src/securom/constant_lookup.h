#pragma once
#include <cstddef>
#include <cstdint>
#include "addresses.h"
#include "log.h"
#include "securom/slot_patch.h"

// Replaces the game's shader constant lookup -> SecuROM slows down the original function
// - binary search in the shader's constant table (8 byte entries, sorted by an unsigned CRC32 at offset 0), returning the matching entry or the end pointer
// - in PC executable SecuROM moved its body into the "Stext" section, obfuscated it, and left a stub in .text: 0x004297B0:  JMP DWORD PTR [0x01DB9218]
// - slot at 0x01DB9218 is plain data (in the .securom section): this points it at an equivalent function of the mod (see slot_patch.h)
namespace ssa::Game::ConstantLookup
{
    inline bool g_verify = false; // also run the game's own function and compare
    inline Securom::VerifyCounters g_verifyCounters;

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
    inline const uint32_t* CallOriginal(void* fn, const uint32_t* first, const uint32_t* last, const uint32_t key)
    {
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

    inline const uint32_t* __cdecl Lookup(const uint32_t* begin, const uint32_t* end, const uint32_t key);

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

    inline Securom::SlotPatch g_patch{"ConstantLookup", CONSTANT_LOOKUP_THUNK, CONSTANT_LOOKUP_SLOT, reinterpret_cast<void*>(&Stub)};

    // called from the stub (plain cdecl)
    inline const uint32_t* __cdecl Lookup(const uint32_t* begin, const uint32_t* end, const uint32_t key)
    {
        const uint32_t* mine = Find(begin, end, key);

        if (g_verify && g_patch.original())
        {
            const uint32_t* theirs = CallOriginal(g_patch.original(), begin, end, key);
            ++g_verifyCounters.compared;
            if (theirs != mine)
            {
                if (g_verifyCounters.mismatches.fetch_add(1) < 8)
                    Log("[ConstantLookup] MISMATCH key=0x%08X begin=%p end=%p mine=%p game=%p", key, begin, end, mine,
                        theirs);
                return theirs; // the game's answer always wins
            }
        }
        return mine;
    }

    // called every frame from Present: keeps the slot in the state the switch asks for
    inline void Update() { g_patch.Update(); }
} // namespace ssa::Game::ConstantLookup
