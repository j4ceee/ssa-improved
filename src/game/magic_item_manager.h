#pragma once

#include <cstddef>
#include <cstdint>
#include "addresses.h"
#include "magic_item.h"

#pragma pack(push, 1)
namespace ssa::Game
{
    struct MagicItemManager
    {
        MagicItem*  m_CurrItem;                 // +0x00 shared_ptr<MagicItem> mPtr -> current active item
        char        _pad0[0x4];                 // +0x04 shared_ptr<MagicItem> mCB -> do not write
        void*       m_pMIHUD;                   // +0x08 UIBase* for HUD pip element
        uint8_t     m_bLevelRestartCheck;       // +0x0C activation gate; set to 1 after a level restart
        char        _pad1[3];                   // +0x0D
        void*       mBrowser_type;              // +0x10 Browser::type (LXB type ptr)
        void*       mBrowser_data;              // +0x14 Browser::data
        void*       m_pModelFile;               // +0x18
        char        m_MagicItemRecordMap[16];   // +0x1C

        static MagicItemManager* instance()
        {
            return reinterpret_cast<MagicItemManager*>(GetAddress(MAGIC_ITEM_MANAGER));
        }

        [[nodiscard]] MagicItem* currItem() const { return m_CurrItem; }
    };
    static_assert(sizeof(MagicItemManager) == 0x2C);
    static_assert(offsetof(MagicItemManager, m_CurrItem) == 0x00);
    static_assert(offsetof(MagicItemManager, m_pMIHUD) == 0x08);
    static_assert(offsetof(MagicItemManager, m_bLevelRestartCheck) == 0x0C);
    static_assert(offsetof(MagicItemManager, mBrowser_type) == 0x10);
    static_assert(offsetof(MagicItemManager, mBrowser_data) == 0x14);
    static_assert(offsetof(MagicItemManager, m_pModelFile) == 0x18);
    static_assert(offsetof(MagicItemManager, m_MagicItemRecordMap) == 0x1C);
} // namespace ssa::Game
#pragma pack(pop)