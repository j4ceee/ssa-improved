#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include "addresses.h"
#include "log.h"
#include "securom/slot_patch.h"

// Replaces the game's function that adds an item to the render list of the NGL scene
// - SecuROM moved the body into its own obfuscated code, where every call costs many times what the function does
namespace ssa::Game::RenderList
{
    inline constexpr size_t kNextOffset = 0x4; // in an item
    inline constexpr size_t kHeadOffset = 0x32C; // in the scene
    inline constexpr size_t kCountOffset = 0x33C; // in the scene

    // what verify mode compares
    inline constexpr size_t kItemSize = 0x20;
    inline constexpr size_t kSceneWindowStart = 0x300;
    inline constexpr size_t kSceneWindowSize = 0x40;
    static_assert(kHeadOffset >= kSceneWindowStart && kCountOffset + sizeof(int) <= kSceneWindowStart + kSceneWindowSize);

    inline bool g_verify = false; // also run the game's function and compare what it changed
    inline Securom::VerifyCounters g_verifyCounters;

    template <class T>
    inline T& At(void* base, const size_t offset)
    {
        return *reinterpret_cast<T*>(static_cast<uint8_t*>(base) + offset);
    }

    inline uint8_t* Scene() { return *reinterpret_cast<uint8_t**>(GetAddress(NGL_SCENE)); }

    inline void Push(uint8_t* scene, uint8_t* item)
    {
        At<void*>(item, kNextOffset) = At<void*>(scene, kHeadOffset);
        At<void*>(scene, kHeadOffset) = item;
        ++At<int>(scene, kCountOffset);
    }

    // game's function through the pointer that was in the slot: EAX = item, EDX = scene (what the callers have there). Keeps the
    // registers of the caller, whatever the game's function does to them.
    inline __declspec(naked) void __cdecl CallOriginal(void* fn, uint8_t* item, uint8_t* scene)
    {
        __asm {
            push ebx
            push esi
            push edi
            push ebp
            mov ecx, [esp + 20] // fn
            mov eax, [esp + 24] // item
            mov edx, [esp + 28] // scene
            call ecx
            pop ebp
            pop edi
            pop esi
            pop ebx
            ret
            }
    }

    inline void __cdecl PushItem(uint8_t* item);

    // entry point for both call sites: EAX = item
    inline __declspec(naked) void Stub()
    {
        __asm {
            push eax
            push ebx
            push ecx
            push edx
            push esi
            push edi
            push ebp
            push dword ptr [esp + 24] // item: the first push, six pushes ago
            call PushItem
            add esp, 4
            pop ebp
            pop edi
            pop esi
            pop edx
            pop ecx
            pop ebx
            pop eax
            ret
            }
    }

    // off at the start: it is only on after a verify run through several levels came back clean
    inline Securom::SlotPatch g_patch{
        "RenderListPush", RENDER_LIST_PUSH_THUNK, RENDER_LIST_PUSH_SLOT, reinterpret_cast<void*>(&Stub), true
    };

    // offset of the first difference between two buffers, or -1
    inline int FirstDifference(const uint8_t* a, const uint8_t* b, const size_t size)
    {
        for (size_t i = 0; i < size; ++i)
            if (a[i] != b[i])
                return static_cast<int>(i);
        return -1;
    }

    // game's function, then a check of what it did
    inline void VerifiedPush(void* original, uint8_t* scene, uint8_t* item)
    {
        uint8_t itemBefore[kItemSize];
        uint8_t sceneBefore[kSceneWindowSize];
        std::memcpy(itemBefore, item, kItemSize);
        std::memcpy(sceneBefore, scene + kSceneWindowStart, kSceneWindowSize);

        CallOriginal(original, item, scene);

        // what the push should have done, applied to the copies
        At<void*>(itemBefore, kNextOffset) = At<void*>(sceneBefore, kHeadOffset - kSceneWindowStart);
        At<void*>(sceneBefore, kHeadOffset - kSceneWindowStart) = item;
        ++At<int>(sceneBefore, kCountOffset - kSceneWindowStart);

        ++g_verifyCounters.compared;
        const int itemDiff = FirstDifference(itemBefore, item, kItemSize);
        const int sceneDiff = FirstDifference(sceneBefore, scene + kSceneWindowStart, kSceneWindowSize);
        if (itemDiff >= 0 || sceneDiff >= 0)
        {
            if (g_verifyCounters.mismatches.fetch_add(1) < 8)
                Log("[RenderListPush] MISMATCH item=%p scene=%p: item differs at +0x%X, scene at +0x%X",
                    static_cast<void*>(item),
                    static_cast<void*>(scene), itemDiff,
                    sceneDiff >= 0 ? static_cast<int>(kSceneWindowStart) + sceneDiff : -1);
        }
    }

    inline void __cdecl PushItem(uint8_t* item)
    {
        uint8_t* scene = Scene();
        void* original = g_patch.original();

        if (!scene)
        {
            CallOriginal(original, item, scene); // not a case we know: the game's function does whatever it does
            return;
        }
        if (g_verify && original)
        {
            VerifiedPush(original, scene, item);
            return;
        }
        Push(scene, item);
    }

    // call every frame from Present
    inline void Update() { g_patch.Update(); }
} // namespace ssa::Game::RenderList
