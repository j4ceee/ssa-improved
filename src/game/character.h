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

    // charExtraBits (+0x1A8) - first word of ltl2::bitset<33,uint>
    static constexpr uint32_t kCharBitDisableCollision = 0x04; // DisableCollision
    static constexpr uint32_t kCharBitDisablePhysics = 0x08; // DisablePhysics
    static constexpr uint32_t kCharBitInvincible = 0x10; // SetInvincible

    // charFlags (+0x1A4)
    static constexpr uint32_t kCharFlagGhost = 0x40000; // SetGhost

    // entity object flags (*m_pObject + 0x60)
    static constexpr uint32_t kEntityFlagNoCollision = 0x02; // set by DisableCollision
    static constexpr uint32_t kEntityFlagNoPhysics = 0x80; // set by DisablePhysics

    struct Character
    {
        char            _pad0[0x00C];       // +0x000
        void*           m_pObject;          // +0x00C entity object ptr - needed for collision/physics double-writes
        char            _pad1[0x10C];       // +0x010
        void*           m_pAttributes;      // +0x11C CharacterAttributes*
        char            _pad2[0x03C];       // +0x120
        uint32_t        nameID;             // +0x15C crc32 - identifies character type
        char            _pad3[0x024];       // +0x160
        int             contID;             // +0x184 controller index (0=P1, 1=P2)
        char            _pad4[0x004];       // +0x188
        void*           m_pPad;             // +0x18C PadController* - non-null = locally controlled
        char            _pad5[0x014];       // +0x190
        uint32_t        charFlags;          // +0x1A4 ghost: kCharFlagGhost
        uint32_t        charExtraBits;      // +0x1A8 invincible/physics/collision bits
        char            _pad6[0x030];       // +0x1AC (includes second word of charExtraBits bitset)
        float           hpMultiplier;       // +0x1DC SetHealth(baseHP * hpMultiplier) at spawn
        float           attackMultiplier;   // +0x1E0 works for evil Skylanders, no-op for regular enemies
        float           baseHP;             // +0x1E4
        char            _pad7[0x0C8];       // +0x1E8
        float           m_fCurrHealth;      // +0x2B0
        char            _pad8[0x044];       // +0x2B4
        CharacterTeam   m_team;             // +0x2F8
        char            _pad9[0x04C];       // +0x2FC
        uint32_t        damageRecMask;      // +0x348
        uint32_t        damageRecMaskPrev;  // +0x34C
        uint32_t        invulnFlags;        // +0x350
        uint32_t        vulnFlags;          // +0x354
        uint8_t         ignoreHitReaction;  // +0x358
        uint8_t         ignoreKnockback;    // +0x359
        char            _pad10[0x008];      // +0x35A
        uint8_t         m_bPvPReady;        // +0x362
        char            _pad11[0x026];      // +0x363
        uint8_t         m_bShieldEnable;    // +0x389

        bool hasGodMode() const { return (charExtraBits & kCharBitInvincible) != 0; }

        void setGodMode(const bool enable)
        {
            if (enable)
                charExtraBits |= kCharBitInvincible;
            else
                charExtraBits &= ~kCharBitInvincible;
        }

        bool hasIgnoreKnockback() const { return ignoreKnockback != 0; }

        void setIgnoreKnockback(const bool enable) { ignoreKnockback = enable ? 1 : 0; }

        bool hasIgnoreHitReaction() const { return ignoreHitReaction != 0; }

        void setIgnoreHitReaction(const bool enable) { ignoreHitReaction = enable ? 1 : 0; }

        bool hasDisableCollision() const { return (charExtraBits & kCharBitDisableCollision) != 0; }

        void setDisableCollision(const bool enable)
        {
            if (!m_pObject) return;
            auto* objFlags = reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(m_pObject) + 0x60);
            if (enable) {
                charExtraBits |= kCharBitDisableCollision;
                *objFlags     |= kEntityFlagNoCollision;
            } else {
                charExtraBits &= ~kCharBitDisableCollision;
                *objFlags     &= ~kEntityFlagNoCollision;
            }
        }

        bool hasDisablePhysics() const { return (charExtraBits & kCharBitDisablePhysics) != 0; }

        void setDisablePhysics(const bool enable)
        {
            if (!m_pObject) return;
            auto* objFlags = reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(m_pObject) + 0x60);
            if (enable) {
                charExtraBits |= kCharBitDisablePhysics;
                *objFlags     |= kEntityFlagNoPhysics;
            } else {
                charExtraBits &= ~kCharBitDisablePhysics;
                *objFlags     &= ~kEntityFlagNoPhysics;
            }
        }

        bool isPlayer() const
        {
            if (!m_pPad) return false;
            // dereference the object pointer to get the vtable pointer
            uintptr_t currentVtable = *reinterpret_cast<uintptr_t*>(m_pPad);
            // compare it against the known PlayerPad vtable address
            return currentVtable == reinterpret_cast<uintptr_t>(GetAddress(PLAYER_PAD_VTABLE));
        }

        bool isLocalAI() const
        {
            if (!m_pPad) return false;
            uintptr_t currentVtable = *reinterpret_cast<uintptr_t*>(m_pPad);
            return currentVtable == reinterpret_cast<uintptr_t>(GetAddress(AI_PAD_VTABLE));
        }

        bool isRemotePlayer() const
        {
            if (!m_pPad) return false;
            uintptr_t currentVtable = *reinterpret_cast<uintptr_t*>(m_pPad);
            return currentVtable == reinterpret_cast<uintptr_t>(GetAddress(REMOTE_PAD_VTABLE));
        }

        bool isEnemy() const { return m_team == CharacterTeam::Enemy; }

        float maxHP() const { return baseHP * hpMultiplier; }

        void healToFull() { m_fCurrHealth = maxHP(); }
    };

    static_assert(offsetof(Character, m_pObject) == 0x00C);
    static_assert(offsetof(Character, m_pAttributes) == 0x11C);
    static_assert(offsetof(Character, nameID) == 0x15C);
    static_assert(offsetof(Character, contID) == 0x184);
    static_assert(offsetof(Character, m_pPad) == 0x18C);
    static_assert(offsetof(Character, charFlags) == 0x1A4);
    static_assert(offsetof(Character, charExtraBits) == 0x1A8);
    static_assert(offsetof(Character, hpMultiplier) == 0x1DC);
    static_assert(offsetof(Character, attackMultiplier) == 0x1E0);
    static_assert(offsetof(Character, baseHP) == 0x1E4);
    static_assert(offsetof(Character, m_fCurrHealth) == 0x2B0);
    static_assert(offsetof(Character, m_team) == 0x2F8);
    static_assert(offsetof(Character, damageRecMask) == 0x348);
    static_assert(offsetof(Character, invulnFlags) == 0x350);
    static_assert(offsetof(Character, ignoreHitReaction) == 0x358);
    static_assert(offsetof(Character, ignoreKnockback) == 0x359);
    static_assert(offsetof(Character, m_bPvPReady) == 0x362);
    static_assert(offsetof(Character, m_bShieldEnable) == 0x389);


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