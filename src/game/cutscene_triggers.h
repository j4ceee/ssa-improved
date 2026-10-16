#pragma once

#include <cstddef>
#include <cstdint>

#include "addresses.h"
#include "component.h"

// Trigger components that run or end cutscenes
#pragma pack(push, 1)
namespace ssa::Game
{
    // base of every trigger
    struct TriggerListener : Component
    {
        char    triggerHandler[0x10];
        uint8_t bInitialized;               // +0x38
        char    _pad39[3];
        uint32_t triggerScriptName;         // +0x3C
        uint8_t bAcceptNullTriggerMessages; // +0x40
        char    _pad41[3];
    };
    static_assert(sizeof(TriggerListener) == 0x44);


    // shared start of CameraAttachTrigger & SpyroCloseUpTrigger
    struct CameraShotTrigger : TriggerListener
    {
        void*   camInst;            // +0x44 non-null while the shot runs
        float   gameSpeed;          // +0x48
        float   interpIn;           // +0x4C
        float   holdTime;           // +0x50
        float   interpOut;          // +0x54
        uint8_t lockControl;        // +0x58 holds the game pause (bit 25) while the shot runs
        uint8_t lockAI;             // +0x59
        uint8_t playerSkip;         // +0x5A skip button ends the shot after 1s
        uint8_t interpOutSkip;      // +0x5B
        uint8_t returnSpyroCamera;  // +0x5C
        char    _pad5D[0x80 - 0x5D];
        float   myTimer;            // +0x80 seconds since the shot started
        uint8_t bFromSkipTrigger;   // +0x84
        char    _pad85[0x3];
        char    trigFinish[0x28];   // +0x88
    };
    static_assert(sizeof(CameraShotTrigger) == 0xB0);
    static_assert(offsetof(CameraShotTrigger, camInst) == 0x44);
    static_assert(offsetof(CameraShotTrigger, lockControl) == 0x58);
    static_assert(offsetof(CameraShotTrigger, playerSkip) == 0x5A);
    static_assert(offsetof(CameraShotTrigger, myTimer) == 0x80);
    static_assert(offsetof(CameraShotTrigger, bFromSkipTrigger) == 0x84);


    struct CameraAttachTrigger : CameraShotTrigger
    {
    };
    static_assert(sizeof(CameraAttachTrigger) == 0xB0);


    struct SpyroCloseUpTrigger : CameraShotTrigger
    {
        char _padB0[0xCC - 0xB0]; // +0xB0 anim / close-up settings
    };
    static_assert(sizeof(SpyroCloseUpTrigger) == 0xCC);


    // level placed, listens for the skip button while the game pause bit 25 is held and forwards its messages
    struct SkipTrigger : TriggerListener
    {
        char forwardSkipTrigger[0x28]; // +0x44
    };
    static_assert(sizeof(SkipTrigger) == 0x6C);


    // scripted camera / actor sequence
    struct ScriptCinema : TriggerListener
    {
        static constexpr int32_t kSkippableMode = 1;

        uint8_t bEnable;        // +0x44
        char    _pad45[3];
        char    trigDone[0x28]; // +0x48
        int32_t mode;           // +0x70
        char    cineList[0x0C]; // +0x74
        uint8_t playerSkip;     // +0x80
        uint8_t mStart;         // +0x81
        char    _pad82[2];
    };
    static_assert(sizeof(ScriptCinema) == 0x84);
    static_assert(offsetof(ScriptCinema, mode) == 0x70);
    static_assert(offsetof(ScriptCinema, playerSkip) == 0x80);


    struct CinemaTrigger
    {
        char    _pad0[0x1F0];   // +0x000
        uint8_t bSeenBefore;    // +0x1F0 - runtime: "already watched"
        uint8_t bNeverSkip;     // +0x1F1 - level data: dev lock, never skippable
        uint8_t bAlwaysSkip;    // +0x1F2 - level data: always skippable

        // mirrors the skip condition in CinemaTrigger::Update / ReallyGo / UnTrigger
        [[nodiscard]] bool canSkip() const { return bAlwaysSkip || (bSeenBefore && !bNeverSkip); }

        static List<CinemaTrigger*>* instanceList()
        {
            return static_cast<List<CinemaTrigger*>*>(GetAddress(CINEMA_TRIGGER_LIST));
        }
    };
    static_assert(offsetof(CinemaTrigger, bSeenBefore) == 0x1F0);
    static_assert(offsetof(CinemaTrigger, bNeverSkip) == 0x1F1);
    static_assert(offsetof(CinemaTrigger, bAlwaysSkip) == 0x1F2);
} // namespace ssa::Game
#pragma pack(pop)
