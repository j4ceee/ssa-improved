#pragma once

#include <cstdint>

#include "addresses.h"
#include "config.h"
#include "log.h"
#include "meow_hook/util.h"
#include "game/game.h"
#include "game/sound_system.h"
#include "game/data/levels.h"
#include "game/data/music.h"

namespace ssa::Game::Music
{
    // -------------------------------------------------------------------------
    // Wwise
    // -------------------------------------------------------------------------
    namespace Wwise
    {
        inline constexpr uint32_t kGameObject = 0x53534150; // custom game object
        inline constexpr uint32_t kCurveLinear = 4;
        inline constexpr uint32_t kActionStop = 0;

        inline uintptr_t Fn(Address a) { return reinterpret_cast<uintptr_t>(GetAddress(a)); }

        // posts a music event on our own game object, returns the playing id (0 = failed)
        inline uint32_t PostMusic(uint32_t eventId)
        {
            static bool s_registered = false;
            if (!s_registered)
            {
                meow_hook::func_call<uint32_t>(Fn(AK_REGISTER_GAME_OBJ), kGameObject);
                s_registered = true;
            }

            float pos[6] = {};
            meow_hook::func_call<uint32_t>(Fn(AK_SET_POSITION), kGameObject, static_cast<void*>(pos), 0xFFFFFFFFu);

            const uint32_t id = meow_hook::func_call<uint32_t>(
                Fn(AK_POST_EVENT), eventId, kGameObject, 0u,
                static_cast<void*>(nullptr), static_cast<void*>(nullptr), 0u, static_cast<void*>(nullptr));

            Log("[Music] PostEvent 0x%08X -> playing id 0x%08X", eventId, id);
            return id;
        }

        inline void Stop(uint32_t playingId, uint32_t fadeMs)
        {
            if (playingId)
                meow_hook::func_call<void>(Fn(AK_STOP_PLAYING_ID), playingId, fadeMs, kCurveLinear);
        }

        // stops a sound the game started: by playing id, by event on its game object and finally everything on that object.
        // the game object of a music sound is its MusicPlayer's own pointer, so nothing else plays on it
        inline void StopGameSound(uint32_t playingId, uint32_t eventId, uint32_t gameObject, uint32_t fadeMs)
        {
            Stop(playingId, fadeMs);
            if (eventId && gameObject)
            {
                meow_hook::func_call<uint32_t>(Fn(AK_EXECUTE_ACTION_ON_EVENT),
                                               eventId, kActionStop, gameObject, fadeMs, kCurveLinear);
                meow_hook::func_call<void>(Fn(AK_STOP_ALL), gameObject);
            }
        }
    }

    // -------------------------------------------------------------------------
    // Main menu: replace the game's menu music with the configured track
    // -------------------------------------------------------------------------
    namespace MenuSwap
    {
        inline constexpr uint32_t kFadeMs = 0;

        struct State
        {
            bool swapped = false; // menu music replaced this menu session
            uint32_t currentEvent = 0; // event we are playing (0 = none)
            uint32_t playingId = 0;
        };

        inline State s_state;

        inline void Update(const Game& game)
        {
            if (game.m_CurrLevel != Data::Levels::FrontEnd.crc)
            {
                s_state = {}; // LevelReset already ran StopAll, our sound is gone
                return;
            }

            const uint32_t wanted = g_config.menuMusicEvent;
            if (wanted == 0) return;

            // user picked another track while in the menu -> switch
            if (s_state.swapped && wanted != s_state.currentEvent)
            {
                Wwise::Stop(s_state.playingId, kFadeMs);
                s_state.playingId = Wwise::PostMusic(wanted);
                s_state.currentEvent = wanted;
                return;
            }
            if (s_state.swapped || wanted == Data::Music::MainMenu.eventId) return;

            // wait until the game has started its own menu music, then replace it
            const Sound* menu = SoundSystem::instance()->findByEvent(Data::Music::MainMenu.eventId);
            if (!menu || !menu->playingId) return;

            Wwise::Stop(menu->playingId, kFadeMs);
            s_state.playingId = Wwise::PostMusic(wanted);
            s_state.currentEvent = wanted;
            s_state.swapped = true; // also on failure, so we don't retry every frame
        }
    }

    // -------------------------------------------------------------------------
    // Hub: the PC / Xbox 360 / PS3 versions play the ruins tracks of the last two stages the wrong way round
    // (the Wii / Wii U versions play the dark track at stage 5 and the final one at stage 6)
    // -------------------------------------------------------------------------
    namespace HubFix
    {
        struct Remap
        {
            int stage; // lux::GetSpyroRuinsState()
            uint32_t from; // event the PC version plays
            uint32_t to; // event the Wii versions play
        };

        inline constexpr Remap kRemaps[] =
        {
            {5, Data::Music::Hub_5.eventId, Data::Music::Hub_4.eventId},
            {6, Data::Music::Hub_4.eventId, Data::Music::Hub_5.eventId},
        };
        inline constexpr uint32_t kFadeMs = 0;

        inline uint32_t Corrected(int stage, uint32_t gameEvent)
        {
            for (const auto& r : kRemaps)
                if (r.stage == stage && r.from == gameEvent) return r.to;
            return gameEvent;
        }

        inline bool s_handled = false; // hub music of this visit already checked

        inline void Update(const Game& game)
        {
            if (game.m_CurrLevel != Data::Levels::Level_Hub.crc)
            {
                s_handled = false;
                return;
            }
            if (s_handled) return;

            // wait until the game has started its hub track
            for (const auto* track : Data::kMusicTracks)
            {
                if (track->category != Data::MusicCategory::Hub) continue;

                const Sound* sound = SoundSystem::instance()->findByEvent(track->eventId);
                if (!sound || !sound->playingId) continue;

                s_handled = true;

                const int32_t stage = *static_cast<const int32_t*>(GetAddress(RUINS_STATE));
                const uint32_t wanted = Corrected(stage, track->eventId);
                if (wanted == track->eventId) return; // nothing to fix at this stage

                Wwise::StopGameSound(sound->playingId, sound->eventId, sound->gameObject, kFadeMs);
                Wwise::PostMusic(wanted);
                Log("[Music] hub stage %d: 0x%08X -> 0x%08X", stage, track->eventId, wanted);
                return;
            }
        }
    }

    inline void Update()
    {
        const Game* game = Game::instance();
        if (!game || game->m_bPrepareNextLevel) return;

        MenuSwap::Update(*game);
        HubFix::Update(*game);
    }
}
