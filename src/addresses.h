#pragma once

namespace ssa
{
    enum Address
    {
        SPYRO_CHARACTER_SETTINGS,
        MP_GAME,
        WORLD,
        GAME,
        MAGIC_ITEM_MANAGER,
        SOUND_MANAGER,
        SOUND_SYSTEM,

        CINEMA_TRIGGER_LIST,
        DEBUG_CAM_UPDATE,
        RETRIEVE_GAME_DATA,
        AIR_MOTION_JUMP,
        CHARACTER_LIST, // Skylanders Character instances only
        CHARACTER_LIST_ALL, // all Character instances (enemies + players + neutral)
        TARGETING_LIST,
        GRASS_COUNT,
        DEFAULT_FOV,

        RUINS_STATE, // int32, hub stage (1-6) cached by lux::GetSpyroRuinsState

        MOUSE_DEVICE,
        // VTables
        PLAYER_PAD_VTABLE,
        AI_PAD_VTABLE,
        REMOTE_PAD_VTABLE,
        PAD_STATE_ARRAY,

        // sound system stuff
        AK_REGISTER_GAME_OBJ, // AK::SoundEngine::RegisterGameObj(uint)
        AK_SET_POSITION, // AK::SoundEngine::SetPosition(uint, AkSoundPosition*, uint)
        AK_POST_EVENT, // AK::SoundEngine::PostEvent(ulong, uint, ulong, cb, void*, ulong, ext*)
        AK_STOP_PLAYING_ID, // AK::SoundEngine::StopPlayingID(ulong, long, AkCurveInterpolation)
        AK_EXECUTE_ACTION_ON_EVENT, // AK::SoundEngine::ExecuteActionOnEvent(ulong event, AkActionOnEventType, uint gameObj, long ms, AkCurveInterpolation)
        AK_STOP_ALL, // AK::SoundEngine::StopAll(uint gameObj)

        // DIRECT GAME HOOKS ARE IMPOSSIBLE DUE TO SECUROM ---------------------------------------
        // input
        // UPDATE_CONTROLLER, // controller input
        //
        // POLL_M_KB, // infinite background loop polling inputs
        // UPDATE_MOUSE, // mouse input handler
        // UPDATE_KEYBOARD, // keyboard input handler
        //
        // // grass drawing
        // GRASS_DRAW_ALL,
        //
        // // sheep
        // SHEEP, // sheep

        COUNT,
    };

    void InitAddresses();
    void* GetAddress(Address address);
} // namespace ssa
