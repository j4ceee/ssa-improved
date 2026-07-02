#pragma once

#include <cstddef>
#include <cstdint>
#include "addresses.h"
#include "data_types.h"

#pragma pack(push, 1)
namespace ssa::Game
{
    struct Character;

    struct CharacterRef
    {
        Character* mPtr;
        void* mCB;
    };

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
        // AnimEntity
        char            _pad0[0x00C];       // +0x000
        void*           m_pObject;          // +0x00C entity object ptr
        char            _pad1a[0x0D0];      // +0x010
        uint32_t        attackStage;        // +0x0E0 1 = active attack
        char            _pad1c[0x034];      // +0x0E4
        void*           m_pAnimStateInterface; // +0x118
        // Character
        void*           m_pAttributes;      // +0x11C CharacterAttributes*
        char            _pad2[0x03C];       // +0x120
        uint32_t        nameID;             // +0x15C crc32 - identifies character type
        char            _pad3[0x024];       // +0x160
        int             contID;             // +0x184 controller index (0=P1, 1=P2)
        char            _pad4[0x004];       // +0x188
        void*           m_pPad;             // +0x18C PadController* - non-null = locally controlled
        char            _pad5a[0x008];      // +0x190
        void*           m_pMotionControl;   // +0x198 MotionControl*, MC+0x2C → PhysicsObject*
        char            _pad5b[0x008];      // +0x19C
        uint32_t        charFlags;          // +0x1A4 ghost: kCharFlagGhost
        uint32_t        charExtraBits;      // +0x1A8 invincible/physics/collision bits
        char            _pad6[0x030];       // +0x1AC (includes second word of charExtraBits bitset)
        float           hpMultiplier;       // +0x1DC SetHealth(baseHP * hpMultiplier) at spawn
        float           attackMultiplier;   // +0x1E0 works for evil Skylanders, no-op for regular enemies
        float           baseHP;             // +0x1E4
        char            _pad7[0x0C8];       // +0x1E8
        float           m_fCurrHealth;      // +0x2B0
        char            _pad8[0x024];       // +0x2B4
        float           m_fLastRemoveHealth;// +0x2D8
        char            _pad8b[0x01C];      // +0x2DC
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

        static List<CharacterRef>* instanceSkylandersList()
        {
            return reinterpret_cast<List<CharacterRef>*>(GetAddress(CHARACTER_LIST));
        }

        static List<CharacterRef>* instanceCharactersList()
        {
            return reinterpret_cast<List<CharacterRef>*>(GetAddress(CHARACTER_LIST_ALL));
        }

        [[nodiscard]] uint8_t* physicsBody() const
        {
            if (!m_pMotionControl) return nullptr;
            return *reinterpret_cast<uint8_t**>(
                static_cast<uint8_t*>(m_pMotionControl) + 0x2C);
        }

        [[nodiscard]] bool hasGodMode() const { return (charExtraBits & kCharBitInvincible) != 0; }

        void setGodMode(const bool enable)
        {
            if (enable)
                charExtraBits |= kCharBitInvincible;
            else
                charExtraBits &= ~kCharBitInvincible;
        }

        [[nodiscard]] bool hasIgnoreKnockback() const { return ignoreKnockback != 0; }

        void setIgnoreKnockback(const bool enable) { ignoreKnockback = enable ? 1 : 0; }

        [[nodiscard]] bool hasIgnoreHitReaction() const { return ignoreHitReaction != 0; }

        void setIgnoreHitReaction(const bool enable) { ignoreHitReaction = enable ? 1 : 0; }

        [[nodiscard]] bool hasDisableCollision() const { return (charExtraBits & kCharBitDisableCollision) != 0; }

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

        [[nodiscard]] bool hasDisablePhysics() const { return (charExtraBits & kCharBitDisablePhysics) != 0; }

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

        [[nodiscard]] bool isPlayer() const
        {
            if (!m_pPad) return false;
            // dereference the object pointer to get the vtable pointer
            uintptr_t currentVtable = *reinterpret_cast<uintptr_t*>(m_pPad);
            // compare it against the known PlayerPad vtable address
            return currentVtable == reinterpret_cast<uintptr_t>(GetAddress(PLAYER_PAD_VTABLE));
        }

        [[nodiscard]] bool isPlayer1() const { return isPlayer() && contID == 0; }

        [[nodiscard]] bool isPlayer2() const { return isPlayer() && contID == 1; }

        [[nodiscard]] bool isLocalAI() const
        {
            if (!m_pPad) return false;
            uintptr_t currentVtable = *reinterpret_cast<uintptr_t*>(m_pPad);
            return currentVtable == reinterpret_cast<uintptr_t>(GetAddress(AI_PAD_VTABLE));
        }

        [[nodiscard]] bool isRemotePlayer() const
        {
            if (!m_pPad) return false;
            uintptr_t currentVtable = *reinterpret_cast<uintptr_t*>(m_pPad);
            return currentVtable == reinterpret_cast<uintptr_t>(GetAddress(REMOTE_PAD_VTABLE));
        }

        [[nodiscard]] bool isEnemy() const { return m_team == CharacterTeam::Enemy; }

        [[nodiscard]] float maxHP() const { return baseHP * hpMultiplier; }

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
    static_assert(offsetof(Character, m_fLastRemoveHealth) == 0x2D8);
    static_assert(offsetof(Character, m_team) == 0x2F8);
    static_assert(offsetof(Character, damageRecMask) == 0x348);
    static_assert(offsetof(Character, invulnFlags) == 0x350);
    static_assert(offsetof(Character, ignoreHitReaction) == 0x358);
    static_assert(offsetof(Character, ignoreKnockback) == 0x359);
    static_assert(offsetof(Character, m_bPvPReady) == 0x362);
    static_assert(offsetof(Character, m_bShieldEnable) == 0x389);
} // namespace ssa::Game
#pragma pack(pop)