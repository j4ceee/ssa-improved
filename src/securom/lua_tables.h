#pragma once
#include <atomic>
#include <cstdint>
#include <cstring>
#include <iterator>
#include "game/lua.h"
#include "log.h"
#include "securom/slot_patch.h"

// Replaces the two functions of the game's Lua that make most of the script engine's time:
// - SecuROM moved the bodies into its own obfuscated code, where every call costs many times what the function does
// - luaH_getstr(table, string): entry of a string key in a table's hash part. Complete replacement: a lookup in the chain of the key's main position or the nil object
// - luaV_gettable(L, t, key, val): `val = t[key]`. Done here: t is a table and the key is a string or an integer in its array part,
//   following __index tables (inheritance) like the game's loop.
//   Passed on to the game's own function: everything that calls back into the game (an __index function), other types of t, other keys, errors.
namespace ssa::Game::LuaTables
{
    using Lua::Node;
    using Lua::String;
    using Lua::Table;
    using Lua::Value;

    inline bool g_verify = false; // also run the game's functions and compare
    inline uint32_t g_getTableDone = 0; // luaV_gettable calls done here (plain counters: not worth an atomic per call)

    // why a luaV_gettable call was passed on to the game's function
    enum class PassedOn : uint8_t
    {
        UserdataIndexed, // t is a userdata (an engine object): its __index is mostly a function
        StringIndexed, // t is a string (s:method())
        OtherTypeIndexed, // t is something else that is not a table
        NumberKeyInHash, // a number key that is not in the array part
        OtherKey, // a key that is neither a string nor a number
        IndexNotATable, // key is not in the table and the metatable's __index is a function (or something else)
        TooManyHops, // more __index tables in a row than the game's loop allows: its function raises the error
        Count,
    };

    inline constexpr const char* kPassedOnNames[] = {
        "t is a userdata", "t is a string", "t is another non-table", "number key in the hash part", "other key type",
        "miss, __index is not a table", "too many __index hops",
    };
    static_assert(std::size(kPassedOnNames) == static_cast<size_t>(PassedOn::Count));
    inline uint32_t g_passedOn[static_cast<size_t>(PassedOn::Count)] = {};

    using VerifyCounters = Securom::VerifyCounters;
    inline VerifyCounters g_getStrVerify;
    inline VerifyCounters g_getTableVerify;

    // -------------------------------------------------------------------------
    // luaH_getstr
    // -------------------------------------------------------------------------

    // entry of `key` in the hash part of `t`, or the nil object
    inline const Value* FindString(const Table* t, const String* key)
    {
        const Lua::Node* n = &t->node[key->hash & ((1u << t->lsizenode) - 1)];
        do
        {
            if (n->keyTt == Lua::TString && n->keyValue.gc == key)
                return &n->val;
            n = n->next;
        }
        while (n);
        return Lua::NilObject();
    }

    // game's function through the pointer that was in the slot: EDX = table, ESI = string, result in EAX. Keeps the registers
    // of the caller, whatever the game's function does to them.
    inline __declspec(naked) const Value* __cdecl CallOriginalGetStr(void* fn, const Table* t, const String* key)
    {
        __asm {
            push ebx
            push esi
            push edi
            push ebp
            mov eax, [esp + 20] // fn
            mov edx, [esp + 24] // t
            mov esi, [esp + 28] // key
            call eax
            pop ebp
            pop edi
            pop esi
            pop ebx
            ret
            }
    }

    inline const Value* __cdecl GetStr(const Table* t, const String* key);

    // entry point of luaH_getstr for all callers: EDX = table, ESI = string, returns in EAX
    inline __declspec(naked) void GetStrStub()
    {
        __asm {
            push ebx
            push ecx
            push esi
            push edi
            push ebp
            push edx
            push esi // key
            push edx // table
            call GetStr
            add esp, 8
            pop edx
            pop ebp
            pop edi
            pop esi
            pop ecx
            pop ebx
            ret
            }
    }

    inline Securom::SlotPatch g_getStr{
        "LuaGetStr", LUA_GETSTR_THUNK, LUA_GETSTR_SLOT, reinterpret_cast<void*>(&GetStrStub)
    };

    inline const Value* __cdecl GetStr(const Table* t, const String* key)
    {
        const Value* mine = FindString(t, key);

        if (g_verify && g_getStr.original())
        {
            const Value* theirs = CallOriginalGetStr(g_getStr.original(), t, key);
            ++g_getStrVerify.compared;
            if (theirs != mine)
            {
                if (g_getStrVerify.mismatches.fetch_add(1) < 8)
                    Log("[LuaGetStr] MISMATCH table=%p key=%p mine=%p game=%p", static_cast<const void*>(t),
                        static_cast<const void*>(key), static_cast<const void*>(mine),
                        static_cast<const void*>(theirs));
                return theirs; // the game's answer always wins
            }
        }
        return mine;
    }

    // -------------------------------------------------------------------------
    // luaV_gettable
    // -------------------------------------------------------------------------

    // entry of the integer key `key` if it is in the array part of `t` (luaH_getnum's first case), otherwise null
    inline const Value* FindInArray(const Table* t, const double key)
    {
        if (!(key >= 1.0 && key <= 2147483647.0))
            return nullptr;
        const int k = static_cast<int>(key);
        if (static_cast<double>(k) != key)
            return nullptr; // not an integer
        if (static_cast<unsigned>(k - 1) >= static_cast<unsigned>(t->sizearray))
            return nullptr; // in the hash part
        return &t->array[k - 1];
    }

    // luaV_gettable's loop count (MAXTAGLOOP)
    inline constexpr int kMaxTagLoop = 100;

    // What luaV_gettable would store, or null with the reason in `why` if the game's function has to do it. Loop is the game's:
    // - t is a table: look the key up. A value that is not nil is the result and so is nil if there is nothing to ask: no metatable,
    //   or its flags say it has no __index
    // - otherwise get the metatable's __index. A table: repeat with it as t. A function (or anything else) calls back into the game: passed on
    inline const Value* TryGet(const void* L, const Value* t, const Value* key, PassedOn& why)
    {
        for (int loop = 0; loop < kMaxTagLoop; ++loop)
        {
            if (t->tt != Lua::TTable)
            {
                why = t->tt == Lua::TUserdata
                          ? PassedOn::UserdataIndexed
                          : t->tt == Lua::TString
                          ? PassedOn::StringIndexed
                          : PassedOn::OtherTypeIndexed;
                return nullptr;
            }
            const auto* h = static_cast<const Table*>(t->gc);

            const Value* result;
            if (key->tt == Lua::TString)
            {
                result = FindString(h, static_cast<const String*>(key->gc));
            }
            else if (key->tt == Lua::TNumber)
            {
                result = FindInArray(h, key->n);
                if (!result)
                {
                    why = PassedOn::NumberKeyInHash;
                    return nullptr;
                }
            }
            else
            {
                why = PassedOn::OtherKey;
                return nullptr;
            }

            if (result->tt != Lua::TNil)
                return result;
            Table* metatable = h->metatable;
            if (!metatable || (metatable->flags & 1))
                return result;

            const Value* index = FindString(metatable, Lua::IndexName(L));
            if (index->tt == Lua::TNil)
            {
                metatable->flags |= 1; // luaT_gettm: remember that there is no __index
                return result;
            }
            if (index->tt != Lua::TTable)
            {
                why = PassedOn::IndexNotATable;
                return nullptr;
            }
            t = index; // repeat with the __index table
        }
        why = PassedOn::TooManyHops;
        return nullptr;
    }

    // setobj: the value and the type, not the padding
    inline void Copy(Value* to, const Value* from)
    {
        const auto* src = reinterpret_cast<const uint32_t*>(from);
        auto* dst = reinterpret_cast<uint32_t*>(to);
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
    }

    // game's function through the pointer that was in the slot (cdecl). Keeps the registers of the caller.
    inline __declspec(naked) void __cdecl CallOriginalGetTable(void* fn, void* L, const Value* t, const Value* key, Value* val)
    {
        __asm {
            push ebx
            push esi
            push edi
            push ebp
            push dword ptr [esp + 36] // val
            push dword ptr [esp + 36] // key
            push dword ptr [esp + 36] // t
            push dword ptr [esp + 36] // L
            call dword ptr [esp + 36] // fn
            add esp, 16
            pop ebp
            pop edi
            pop esi
            pop ebx
            ret
            }
    }

    inline void __cdecl GetTable(void* L, const Value* t, const Value* key, Value* val);

    // entry point of luaV_gettable for all callers: cdecl (L, t, key, val), returns nothing
    inline __declspec(naked) void GetTableStub()
    {
        __asm {
            push ebx
            push ecx
            push edx
            push esi
            push edi
            push ebp
            push dword ptr [esp + 40] // val
            push dword ptr [esp + 40] // key
            push dword ptr [esp + 40] // t
            push dword ptr [esp + 40] // L
            call GetTable
            add esp, 16
            pop ebp
            pop edi
            pop esi
            pop edx
            pop ecx
            pop ebx
            ret
            }
    }

    inline Securom::SlotPatch g_getTable{
        "LuaGetTable", LUA_GETTABLE_THUNK, LUA_GETTABLE_SLOT, reinterpret_cast<void*>(&GetTableStub)
    };

    inline void __cdecl GetTable(void* L, const Value* t, const Value* key, Value* val)
    {
        PassedOn why = PassedOn::OtherKey;
        const Value* result = TryGet(L, t, key, why);
        if (!result)
        {
            ++g_passedOn[static_cast<size_t>(why)];
            CallOriginalGetTable(g_getTable.original(), L, t, key, val);
            return;
        }

        ++g_getTableDone;

        if (g_verify && g_getTable.original())
        {
            // done here means no call back into the game, so the game's function can fill a copy first
            Value theirs = {};
            CallOriginalGetTable(g_getTable.original(), L, t, key, &theirs);
            ++g_getTableVerify.compared;
            if (theirs.tt != result->tt || std::memcmp(&theirs, result, sizeof(theirs.n)) != 0)
            {
                if (g_getTableVerify.mismatches.fetch_add(1) < 8)
                    Log("[LuaGetTable] MISMATCH t=%p key=%p (type %d) mine: type %d, game: type %d",
                        static_cast<const void*>(t),
                        static_cast<const void*>(key), key->tt, result->tt, theirs.tt);
                Copy(val, &theirs); // the game's answer always wins
                return;
            }
        }
        Copy(val, result);
    }

    // call every frame from Present
    inline void Update()
    {
        g_getStr.Update();
        g_getTable.Update();
    }
} // namespace ssa::Game::LuaTables
