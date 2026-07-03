#pragma once

#include "config.h"
#include "game/magic_item_manager.h"

namespace ssa::Game::ItemMods
{
    // called from hook_Present every frame
    inline void Update()
    {
        auto* mgr = MagicItemManager::instance();
        if (!mgr) return;

        if (g_config.infiniteItemDuration)
        {
            if (auto* item = mgr->currItem())
                item->makeTimedItemPersist();
        }

        if (g_config.reusableItems)
        {
            if (auto* item = mgr->currItem())
                item->allowItemReuse();
        }
    }
}
