#pragma once

namespace ssa
{
    enum Address
    {
        MOUSE_DEVICE,

        SPYRO_CHARACTER_SETTINGS,
        CHARACTER_LIST,
        CHARACTER_LIST_ALL, // all Character instances (enemies + players + neutral)
        GRASS_COUNT,

        // DIRECT GAME HOOKS ARE IMPOSSIBLE DUE TO SECUROM ---------------------------------------
        // input
        UPDATE_CONTROLLER, // controller input

        POLL_M_KB, // infinite background loop polling inputs
        UPDATE_MOUSE, // mouse input handler
        UPDATE_KEYBOARD, // keyboard input handler

        // grass drawing
        GRASS_DRAW_ALL,

        // sheep
        SHEEP, // sheep

        COUNT,
    };

    void InitAddresses();
    void* GetAddress(Address address);
} // namespace ssa
