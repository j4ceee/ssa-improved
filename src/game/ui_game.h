#pragma once

#include <cstddef>
#include <cstdint>

#include "addresses.h"
#include "data_types.h"
#include "ui_dialog.h"

#pragma pack(push, 1)
namespace ssa::Game
{
    struct UIGame
    {
        char _pad0[0x3D8];
        uint32_t dialogCount; // +0x3D8
        List<UIDialog*> dialogs; // +0x3DC head = the dialog being updated, rest are queued

        static UIGame* instance() { return static_cast<UIGame*>(GetAddress(UI_GAME)); }
    };

    static_assert(offsetof(UIGame, dialogCount) == 0x3D8);
    static_assert(offsetof(UIGame, dialogs) == 0x3DC);
} // namespace ssa::Game
#pragma pack(pop)
