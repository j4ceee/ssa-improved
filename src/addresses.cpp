#include "addresses.h"

namespace ssa
{
    static long long g_Address[COUNT] = {};

    void InitAddresses()
    {
        g_Address[GRASS_DRAW_ALL] = 0x00985c30;
    }

    void* GetAddress(Address address)
    {
        return reinterpret_cast<void*>(g_Address[address]);
    }
} // namespace ssa
