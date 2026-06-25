#include "addresses.h"

namespace ssa
{
    static long long g_Address[COUNT] = {};

    void InitAddresses()
    {
        g_Address[MOUSE_DEVICE] = 0x00ca29e0; // DirectInput mouse device pointer

        g_Address[SPYRO_CHARACTER_SETTINGS] = 0x00e55420;
        g_Address[CHARACTER_LIST] = 0x00ca2a54;
        g_Address[CHARACTER_LIST_ALL] = 0x00ca2a48;
        g_Address[TARGETING_LIST] = 0x00e3ddf0;
        g_Address[GRASS_COUNT] = 0x00d9146c; // number of grass patches to draw (probably / maybe?)

        // VTables
        g_Address[PLAYER_PAD_VTABLE] = 0x00bb6414;
        g_Address[AI_PAD_VTABLE]     = 0x00bb645c;
        g_Address[REMOTE_PAD_VTABLE] = 0x00bb64a4;

        // DIRECT GAME HOOKS ARE IMPOSSIBLE DUE TO SECUROM ---------------------------------------
        g_Address[UPDATE_CONTROLLER] = 0x009475f0;

        g_Address[POLL_M_KB] = 0x00945df0; // loops infinitely in the background, no MinHook detouring!!
        g_Address[UPDATE_MOUSE] = 0x009471a0;
        g_Address[UPDATE_KEYBOARD] = 0x00946570;

        g_Address[GRASS_DRAW_ALL] = 0x00985c30;

        g_Address[SHEEP] = 0x00a30330; // sheep
    }

    void* GetAddress(Address address)
    {
        return reinterpret_cast<void*>(g_Address[address]);
    }
} // namespace ssa
