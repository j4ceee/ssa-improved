#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <meow_hook/util.h>
#include "addresses.h"
#include "component.h"
#include "data_types.h"

#pragma pack(push, 1)
namespace ssa::Game
{
    static constexpr uint32_t kCrcLight = 0xAADB73F3; // EntityObject::findComponent key of lux::Light

    struct LxContext;
    struct LxBrowser;

    // - factory attaches it to an entity object (Component::owner) and registers it in one of the world's light lists
    // - allocated by the engine, so it must never be freed by the mod: destroy() + releasing references is the only way out
    struct Light
    {
        Component base;     // +0x000
        char _pad0[0x58];   // +0x028
        float colour[4];    // +0x080 linear HDR RGB
        float colour2[4];   // +0x090
        float position[4];  // +0x0A0 derived from owner
        float direction[4]; // +0x0B0 derived from owner
        int32_t lightType;  // +0x0C0 4 = omni
        float range;        // +0x0C4
        float range2;       // +0x0C8
        float attenuate;    // +0x0CC
        float attenuate2;   // +0x0D0
        float hotspot;      // +0x0D4
        float falloff;      // +0x0D8
        float cosHalfHotspot; // +0x0DC
        float cosHalfFalloff; // +0x0E0
        float tanHalfFalloff; // +0x0E4
        float offset;       // +0x0E8
        int32_t affectDiffuse; // +0x0EC
        int32_t affectSpecular; // +0x0F0
        int32_t ambientOnly; // +0x0F4

        // engine clears Component::self when it deletes the light (owner teardown, expiry, level change)
        [[nodiscard]] bool isDeleted() const { return !base.self.ptr; }

        // Light::Delete (vtable slot 1): unlinks the light from its owner and the light lists, drops the engine's own reference
        // object stays valid as long as someone holds a strong reference, but must not be deleted twice
        void destroy()
        {
            const auto* vtable = static_cast<const uintptr_t*>(GetAddress(LIGHT_VTABLE));
            meow_hook::func_call<void>(vtable[1], this);
        }

        // lux::Light::LightFromLX: parses the definition behind `browser`, builds a light on context.owner and returns it (null on failure)
        // - everything is read synchronously, the context and browser data only have to outlive the call
        [[nodiscard]] static Light* createFromLX(LxContext& context, const LxBrowser& browser);
    };

    static_assert(offsetof(Light, colour) == 0x80);
    static_assert(offsetof(Light, position) == 0xA0);
    static_assert(offsetof(Light, lightType) == 0xC0);
    static_assert(offsetof(Light, range) == 0xC4);
    static_assert(offsetof(Light, offset) == 0xE8);
    static_assert(offsetof(Light, ambientOnly) == 0xF4);

    // lux::Browser: cursor into LX data, a type descriptor plus the value it describes
    struct LxBrowser
    {
        const void* type;
        const void* value;
    };

    // part of the model loader's context that Light::LightFromLX reads
    struct LxContext
    {
        uint32_t reserved[2] = {}; // +0x00
        SharedPtr<EntityObject> owner = {}; // +0x08
        int32_t matrixIndex = 0; // +0x10
        RotTransMat4x3FP matrix = {}; // +0x14 only used if there is no owner
    };

    static_assert(offsetof(LxContext, owner) == 0x08);
    static_assert(offsetof(LxContext, matrixIndex) == 0x10);
    static_assert(offsetof(LxContext, matrix) == 0x14);

    inline Light* Light::createFromLX(LxContext& context, const LxBrowser& browser)
    {
        using Factory = Light*(__cdecl*)(LxContext*, const LxBrowser*);
        return reinterpret_cast<Factory>(reinterpret_cast<uintptr_t>(GetAddress(LIGHT_FACTORY)))(&context, &browser);
    }
} // namespace ssa::Game
#pragma pack(pop)
