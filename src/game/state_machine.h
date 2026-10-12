#pragma once

#include <cstddef>
#include <cstdint>
#include "addresses.h"

#pragma pack(push, 1)
namespace ssa::Game
{
    // one state of a character's state machine
    struct State
    {
        const void* vtable; // +0x00 address identifies the class

        [[nodiscard]] bool isClass(const Address vtableAddress) const
        {
            return vtable == GetAddress(vtableAddress);
        }
    };

    struct StateMachineInstance
    {
        char _pad0[0x18]; // +0x000
        State* m_pCurrState; // +0x018

        // is the current state an instance of the class with this vtable?
        [[nodiscard]] bool isInState(const Address stateVtable) const
        {
            return m_pCurrState && m_pCurrState->isClass(stateVtable);
        }
    };
    static_assert(offsetof(StateMachineInstance, m_pCurrState) == 0x018);
} // namespace ssa::Game
#pragma pack(pop)
