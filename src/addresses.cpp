#include "addresses.h"

namespace ssa
{
static uintptr_t g_Address[Address::COUNT] = {};

void InitAddresses()
{
    g_Address[DI_OUTPUT_REPORT]     = (0x0070E2B0);
    g_Address[DI_DISCONNECTED]      = (0x0070AA10);
    g_Address[DI_UPDATE]            = (0x0070C850);
    g_Address[DI_PORTAL_DATA]       = (0x0070C800);
}

uintptr_t GetAddress(Address address)
{
    return g_Address[address];
}
} // namespace ssa