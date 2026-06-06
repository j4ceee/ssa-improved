#include "addresses.h"

namespace ssa
{
    static long long g_Address[COUNT] = {};

    void InitAddresses()
    {
        g_Address[MOUSE_DEVICE] = 0x00ca29e0; // DirectInput mouse device pointer
        g_Address[GRASS_COUNT] = 0x00d9146c; // number of grass patches to draw (probably / maybe?)


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
