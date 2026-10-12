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
        DEFAULT_FOV,

        RUINS_STATE, // int32, hub stage (1-6) cached by lux::GetSpyroRuinsState

        // movement stuff
        CHECK_CAN_MOVE,
        MOTION_CONTROL_TELEPORT,
        CHARACTER_FORCE_STATE,
        PLAYER_LEASH_MAX,
        RESPAWN_POINT_LIST,

        // dynamic lights
        LIGHT_FACTORY,
        LIGHT_DELETE, // lux::Light::Delete (thiscall, vtable slot 1)
        SHARED_CTRL_ON_STRONG_ZERO,
        SHARED_CTRL_ON_WEAK_ZERO,

        // SecuROM moved the game's shader constant lookup into its own code, see securom/constant_lookup.h
        CONSTANT_LOOKUP_THUNK, // stub in .text: JMP [CONSTANT_LOOKUP_SLOT]
        CONSTANT_LOOKUP_SLOT, // pointer to the function body, plain data in the .securom section

        // SecuROM moved parts of the game's Lua 5.1 into its own code, see securom/lua_tables.h
        LUA_NIL_OBJECT, // luaO_nilobject, what a failed table lookup returns
        LUA_GETTABLE_THUNK,
        LUA_GETTABLE_SLOT,
        LUA_GETSTR_THUNK,
        LUA_GETSTR_SLOT,

        // the NGL scene keeps a list of render items; SecuROM protected the function that adds one, see securom/render_list.h
        NGL_SCENE, // pointer to the scene object (list head at +0x32C, item count at +0x33C)
        RENDER_LIST_PUSH_THUNK, // push the item in EAX on that list: JMP [RENDER_LIST_PUSH_SLOT]
        RENDER_LIST_PUSH_SLOT,

        // the game reads a 4x4 system memory texture (a tiny copy of the scene) back to the CPU every frame to build a histogram of it, see window/async_readback.h
        READBACK_LOCK_RETURN, // return address of its IDirect3DTexture9::LockRect call

        JOY_POLL_RETURN,
        MOUSE_DEVICE,
        // VTables
        PLAYER_PAD_VTABLE,
        AI_PAD_VTABLE,
        REMOTE_PAD_VTABLE,
        LIGHT_VTABLE,
        PROJECTILE_VTABLE,
        PAD_STATE_ARRAY,
        STATE_DROWN_VTABLE,
        STATE_WAIT_JOIN_VTABLE,

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
        // // sheep
        // SHEEP, // sheep

        COUNT,
    };

    void InitAddresses();
    void* GetAddress(Address address);
} // namespace ssa
