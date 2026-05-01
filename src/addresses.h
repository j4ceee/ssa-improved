#pragma once

#include <cstdint>

namespace ssa
{
enum Address {
    DI_OUTPUT_REPORT,
    DI_DISCONNECTED,
    DI_UPDATE,
    DI_PORTAL_DATA,
    COUNT,
};

void InitAddresses();
uintptr_t GetAddress(Address address);
}; // namespace ssa