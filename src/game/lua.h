#pragma once
#include <cstddef>
#include <cstdint>
#include "addresses.h"

// game's Lua 5.1
namespace ssa::Game::Lua
{
    enum Type : int
    {
        TNil = 0,
        TBoolean = 1,
        TLightUserdata = 2,
        TNumber = 3,
        TString = 4,
        TTable = 5,
        TFunction = 6,
        TUserdata = 7,
    };

    // TValue: value (8 bytes, a double in it) & its type. 16 bytes, so the stack and arrays are indexed with a shift of 4
    struct Value
    {
        union
        {
            void* gc;
            double n;
            int b;
        };

        int tt;
    };

    static_assert(sizeof(Value) == 16);
    static_assert(offsetof(Value, tt) == 0x8);

    struct String
    {
        void* next;
        uint8_t tt;
        uint8_t marked;
        uint8_t reserved;
        uint32_t hash;
        uint32_t len;
    };

    static_assert(offsetof(String, hash) == 0x8);

    struct Node
    {
        Value val; // value of the entry, so a Node* is also a Value*
        union
        {
            void* gc;
            double n;
        } keyValue;

        int keyTt;
        Node* next; // the next entry with the same main position
    };

    static_assert(sizeof(Node) == 0x20);
    static_assert(offsetof(Node, keyValue) == 0x10);
    static_assert(offsetof(Node, keyTt) == 0x18);
    static_assert(offsetof(Node, next) == 0x1C);

    struct Table
    {
        void* next;
        uint8_t tt;
        uint8_t marked;
        uint8_t flags; // bit n set: the metatable is known to have no tag method n (bit 0 = __index)
        uint8_t lsizenode; // the hash part has 2^lsizenode nodes
        Table* metatable;
        Value* array;
        Node* node;
        Node* lastfree;
        void* gclist;
        int sizearray;
    };

    static_assert(offsetof(Table, flags) == 0x6);
    static_assert(offsetof(Table, lsizenode) == 0x7);
    static_assert(offsetof(Table, metatable) == 0x8);
    static_assert(offsetof(Table, array) == 0xC);
    static_assert(offsetof(Table, node) == 0x10);
    static_assert(offsetof(Table, sizearray) == 0x1C);

    // lua_State: its global state and in it the name string of the __index tag method (offsets from luaV_gettable / luaV_settable)
    inline const uint8_t* GlobalState(const void* L)
    {
        return *reinterpret_cast<const uint8_t* const*>(static_cast<const uint8_t*>(L) + 0x10);
    }

    inline const String* IndexName(const void* L)
    {
        return *reinterpret_cast<const String* const*>(GlobalState(L) + 0xBC);
    }

    // luaO_nilobject: what a lookup returns for a key that is not in the table
    inline const Value* NilObject() { return reinterpret_cast<const Value*>(GetAddress(LUA_NIL_OBJECT)); }
} // namespace ssa::Game::Lua
