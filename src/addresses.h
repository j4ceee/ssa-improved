#pragma once

namespace ssa
{
    enum Address
    {
        GRASS_DRAW_ALL,
        COUNT,
    };

    void InitAddresses();
    void* GetAddress(Address address);
} // namespace ssa
