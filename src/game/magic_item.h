#pragma once

#include <cstddef>
#include <cstdint>

#pragma pack(push, 1)
namespace ssa::Game
{
    enum class Type : uint32_t
    {
        Lantern = 0x2E9822FDu,
        Potion = 0x13CACB04u,
        Shield = 0x47EFA941u,
        Zapper = 0x7F1AED6Eu,
        Hourgrass = 0x4B9A07BBu,
        Anvil = 0xD68857F3u, // -0x2977810D
        Pirates = 0x48344A19u,
        Location_A = 0x08C2FCC0u, // adventure-pack level unlocks (instant, no timer)
        Location_B = 0x5EA1DCD4u,
        Location_C = 0x793797A5u,
        Location_D = 0xA913AC1Bu, // -0x56EC53E5
        Regeneration = 0xA0E6BB56u, // -0x5F1944AA
        SecretStash = 0xC1738D3Bu, // -0x3E8C72C5
        PVPLevel = 0xF1C7FC2Du, // -0x0E3803D3
        Pet_A = 0x29CC05F3u,
        Pet_B = 0xC7C264DFu, // -0x383D9B21
        Pet_C = 0xB0C55449u, // -0x4F3AABB7
        Pet_D = 0x5ECB3565u,
    };

    inline bool isTimedType(Type t)
    {
        switch (t)
        {
            case Type::Location_A:
            case Type::Location_B:
            case Type::Location_C:
            case Type::Location_D:
            case Type::SecretStash:
            case Type::PVPLevel:
                return false;
            default:
                return true;
        }
    }

    struct MagicItem
    {
        void*       __vt;                       // +0x00 vtable
        uint32_t    m_Type;                     // +0x04 Type CRC32
        uint32_t    m_Id;                       // +0x08 item instance CRC32
        char        m_Name[32];                 // +0x0C display name (null-terminated)
        float       m_Lifetime;                 // +0x2C total duration in seconds (0 for instant items)
        float       m_Timer;                    // +0x30 current countdown; decremented each frame by MagicItem::Update
        uint8_t     b_TimeFlag;                 // +0x34 1 while active/timing, 0 after TimesUp/Exit
        uint8_t     b_Active;                   // +0x35
        uint8_t     b_Persist;                  // +0x36 suppresses deactivation callback on timer expiry
        uint8_t     b_Used;                     // +0x37 set by SwitchMagicItem when restoring saved timer state
        uint32_t    m_EleType;                  // +0x38 lux::Spyro_ElementalType
        uint8_t     b_Hidden;                   // +0x3C
        uint8_t     b_IsPet;                    // +0x3D
        char        _pad0[2];                   // +0x3E
        uint32_t    m_MagicIntroSoundName;      // +0x40 CRC32
        uint32_t    m_MagicShortIntroSoundName; // +0x44 CRC32
        uint32_t    m_MagicUsedSoundName;       // +0x48 CRC32
        void*       m_MagicIntroSound_ptr;      // +0x4C shared_ptr<Sound> mPtr
        void*       m_MagicIntroSound_cb;       // +0x50 shared_ptr<Sound> mCB
        void*       m_MagicShortIntroSound_ptr; // +0x54
        void*       m_MagicShortIntroSound_cb;  // +0x58
        void*       m_MagicUsedSound_ptr;       // +0x5C
        void*       m_MagicUsedSound_cb;        // +0x60
        int32_t     m_OwnerID;                  // +0x64

        [[nodiscard]] Type type() const { return static_cast<Type>(m_Type); }
        [[nodiscard]] bool isActive() const { return b_Active != 0; }
        [[nodiscard]] bool isTimed() const { return m_Lifetime > 0.0f; }
        [[nodiscard]] const char* name() const { return m_Name; }

        void resetTimer() // makes item last forever with ui progress bar shown (but never decreasing)
        {
            if (isTimed())
                m_Timer = m_Lifetime;
        }

        void makeTimedItemPersist() // makes item last forever without any ui progress bar displayed
        {
            if (isTimed())
            {
                m_Lifetime = -1;
                m_Timer = -1;
                b_Persist = 1;
                b_Used = 0;
            }
        }

        void allowItemReuse()
        {
            if (b_Used == 1 && b_IsPet == 0)
            {
                b_Used = 0;
                if (isTimed())
                {
                    m_Timer = m_Lifetime;
                }
            }
        }
    };
    static_assert(sizeof(MagicItem) == 0x68);
    static_assert(offsetof(MagicItem, __vt) == 0x00);
    static_assert(offsetof(MagicItem, m_Type) == 0x04);
    static_assert(offsetof(MagicItem, m_Id) == 0x08);
    static_assert(offsetof(MagicItem, m_Name) == 0x0C);
    static_assert(offsetof(MagicItem, m_Lifetime) == 0x2C);
    static_assert(offsetof(MagicItem, m_Timer) == 0x30);
    static_assert(offsetof(MagicItem, b_TimeFlag) == 0x34);
    static_assert(offsetof(MagicItem, b_Active) == 0x35);
    static_assert(offsetof(MagicItem, b_Persist) == 0x36);
    static_assert(offsetof(MagicItem, b_Used) == 0x37);
    static_assert(offsetof(MagicItem, m_EleType) == 0x38);
    static_assert(offsetof(MagicItem, b_Hidden) == 0x3C);
    static_assert(offsetof(MagicItem, b_IsPet) == 0x3D);
    static_assert(offsetof(MagicItem, m_MagicIntroSoundName) == 0x40);
    static_assert(offsetof(MagicItem, m_MagicShortIntroSoundName) == 0x44);
    static_assert(offsetof(MagicItem, m_MagicUsedSoundName) == 0x48);
    static_assert(offsetof(MagicItem, m_MagicIntroSound_ptr) == 0x4C);
    static_assert(offsetof(MagicItem, m_MagicShortIntroSound_ptr) == 0x54);
    static_assert(offsetof(MagicItem, m_MagicUsedSound_ptr) == 0x5C);
    static_assert(offsetof(MagicItem, m_OwnerID) == 0x64);
} // namespace ssa::Game
#pragma pack(pop)