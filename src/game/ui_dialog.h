#pragma once

#include <cstddef>
#include <cstdint>

#include "addresses.h"
#include "data_types.h"

#pragma pack(push, 1)
namespace ssa::Game
{
    struct UIDialog
    {
        void*       vtable;                 // +0x00
        int32_t     type;                   // +0x04 UIDialogType (4 / 5 = question pages with their own button handling)
        int32_t     state;                  // +0x08 0 = opening, 1 = running, 2 = closing
        uint8_t     done;                   // +0x0C set to 1 = dialogue ends (state 1 -> 2 -> Done -> Kill)
        char        _pad0D[0x3];
        int32_t     handle;                 // +0x10
        char        _pad14[0x8];            // +0x14
        uint32_t    textID;                 // +0x1C
        int32_t     voID;                   // +0x20
        char        _pad24[0x10];           // +0x24
        float       totalTime;              // +0x34
        float       holdTime;               // +0x38
        uint8_t     bPauseAllWithoutDialog; // +0x3C
        uint8_t     bLockControl;           // +0x3D
        uint8_t     bPause;                 // +0x3E
        uint8_t     bInteractive;           // +0x3F can be dismissed with the confirm button
        uint8_t     bSetAIThinking;         // +0x40
        uint8_t     excludePlayerInput[2];  // +0x41
        char        _pad43;
        float       currTime;               // +0x44
        uint8_t     bTalkOver;              // +0x48
        uint8_t     voiceOnly;              // +0x49
        char        _pad4A[0x2];
        int32_t     voCharacter;            // +0x4C
        uint8_t     isKilled;               // +0x50
        uint8_t     waitLoader;             // +0x51
        char        _pad52[0x2];
        SharedPtr<void> spVoice;            // +0x54 voice line
        char        _pad5C[0x30];

        [[nodiscard]] bool isTextDialog() const
        {
            return vtable == GetAddress(UI_TEXT_DIALOG_VTABLE);
        }
    };
    static_assert(sizeof(UIDialog) == 0x8C);
    static_assert(offsetof(UIDialog, done) == 0x0C);
    static_assert(offsetof(UIDialog, textID) == 0x1C);
    static_assert(offsetof(UIDialog, bPause) == 0x3E);
    static_assert(offsetof(UIDialog, bInteractive) == 0x3F);
    static_assert(offsetof(UIDialog, excludePlayerInput) == 0x41);
    static_assert(offsetof(UIDialog, bTalkOver) == 0x48);
    static_assert(offsetof(UIDialog, voiceOnly) == 0x49);
    static_assert(offsetof(UIDialog, spVoice) == 0x54);
} // namespace ssa::Game
#pragma pack(pop)