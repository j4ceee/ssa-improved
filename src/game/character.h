#pragma once

#include <cstddef>
#include <cstdint>

#include "addresses.h"

#pragma pack(push, 1)
namespace ssa::Game
{
    // CRC values confirmed from Character::Init
    enum class CharacterTeam : int
    {
        Enemy       = -0x5AB23DB9, // enemy team - unconditional targeting setup
        Skylander   = -0x6AE508FA, // player team - gets IsPvPLevel check in Init
        Neutral     = -0x15D6E3E3, // third team type
    };

    struct Character
    {
        char            _pad0[0x11C];       // +0x000
        void*           m_pAttributes;      // +0x11C  CharacterAttributes* - null for boss Characters (bosses bypass EnemySettings because of this)
        char            _pad1[0x3C];        // +0x120
        uint32_t        nameID;             // +0x15C crc32 - identifies which character type this is
        char            _pad2[0x24];        // +0x160
        int             contID;             // +0x184 controller index (0=P1, 1=P2); verified in-game
        char            _pad3[0x4];         // +0x188
        void*           m_pPad;             // +0x18C PadController* - non-null = locally controlled
        char            _pad4[0x4C];        // +0x190
        float           hpMultiplier;       // +0x1DC Init calls SetHealth(baseHP * hpMultiplier)
        float           attackMultiplier;   // +0x1E0
        float           baseHP;             // +0x1E4 Init writes this directly
        char            _pad5[0xC8];        // +0x1E8
        float           m_fCurrHealth;      // +0x2B0
        char            _pad6[0x44];        // +0x2B4
        CharacterTeam   m_team;             // +0x2F8
    };

    static_assert(offsetof(Character, contID)           == 0x184);
    static_assert(offsetof(Character, m_pPad)           == 0x18C);
    static_assert(offsetof(Character, hpMultiplier)     == 0x1DC);
    static_assert(offsetof(Character, attackMultiplier) == 0x1E0);
    static_assert(offsetof(Character, baseHP)           == 0x1E4);
    static_assert(offsetof(Character, m_fCurrHealth)    == 0x2B0);
    static_assert(offsetof(Character, m_team)           == 0x2F8);

    // -------------------------------------------------------------------------
    // Character list - ltl2::list<weak_ptr<Character>>
    //
    // The sentinel (list_node_base) lives at a fixed data address.
    // Real nodes are heap-allocated list_node<weak_ptr<Character>>:
    //   +0x00  prev*
    //   +0x04  next*
    //   +0x08  Character* (weak_ptr.mPtr)  ← only on real nodes, NOT on sentinel
    //   +0x0C  control_block*
    //
    // Always compare against end() before dereferencing ptr.
    // -------------------------------------------------------------------------

    struct CharacterListNode
    {
        CharacterListNode*  prev; // +0x00
        CharacterListNode*  next; // +0x04
        Character*          ptr;  // +0x08  weak_ptr.mPtr - invalid on sentinel
        void*               cb;   // +0x0C  control_block*
    };

    struct CharacterList
    {
        // list_node_base: just the two link pointers, no data
        CharacterListNode* m_prev; // +0x00 last node  (or self if empty)
        CharacterListNode* m_next; // +0x04 first node (or self if empty)

        [[nodiscard]] CharacterListNode* begin() const
        {
            return m_next;
        }

        // end() returns a pointer to the sentinel cast as a node -
        // only ever used for identity comparison, never dereferenced
        [[nodiscard]] CharacterListNode* end() const
        {
            return reinterpret_cast<CharacterListNode*>(const_cast<CharacterList*>(this));
        }

        static CharacterList* instanceSkylanders()
        {
            return reinterpret_cast<CharacterList*>(GetAddress(CHARACTER_LIST));
        }

        static CharacterList* instanceAll()
        {
            return reinterpret_cast<CharacterList*>(GetAddress(CHARACTER_LIST_ALL));
        }
    };

} // namespace ssa::Game
#pragma pack(pop)