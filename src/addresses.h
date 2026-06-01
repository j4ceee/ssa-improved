#pragma once

namespace ssa
{
    enum Address
    {
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
