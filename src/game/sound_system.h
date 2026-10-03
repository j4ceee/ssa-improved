#pragma once

#include <cstddef>
#include <cstdint>
#include "addresses.h"
#include "data_types.h"

#pragma pack(push, 1)
namespace ssa::Game
{
    struct Sound
    {
        uint32_t eventId;       // +0x00 event id posted by Sound::Start
        uint32_t gameObject;    // +0x04 game object id
        uint32_t playingId;     // +0x08 PostEvent result (0 = failed)
    };
    static_assert(offsetof(Sound, playingId) == 0x08);


    struct SoundSystem
    {
        char _pad0[0x34];
        List<SharedPtr<Sound>> playing; // +0x34 sounds currently playing

        static SoundSystem* instance()
        {
            return reinterpret_cast<SoundSystem*>(GetAddress(SOUND_SYSTEM));
        }

        // first playing sound for the given Wwise event, or nullptr
        Sound* findByEvent(uint32_t eventId)
        {
            for (const auto& s : playing)
                if (s.ptr && s.ptr->eventId == eventId)
                    return s.ptr;
            return nullptr;
        }
    };
    static_assert(offsetof(SoundSystem, playing) == 0x34);


    struct SoundManager
    {
        char        _pad0[0x2C];    // +0x00
        uint32_t    musicStateId;   // +0x2C

        static SoundManager* instance()
        {
            return reinterpret_cast<SoundManager*>(GetAddress(SOUND_MANAGER));
        }
    };
    static_assert(offsetof(SoundManager, musicStateId) == 0x2C);

} // namespace ssa::Game
#pragma pack(pop)