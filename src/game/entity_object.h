#pragma once

#include <cstddef>
#include <cstdint>

#include "data_types.h"

#pragma pack(push, 1)
namespace ssa::Game
{
    struct ComponentNode
    {
        uintptr_t       prevTagged; // +0x0
        ComponentNode*  next;       // +0x4
        uint32_t        crc;        // +0x8
        void*           component;  // +0xC
    };

    struct EntityObject
    {
        char        _pad0[0x34];    // +0x000
        Vec3        position;       // +0x034
        char        _pad1[0x20];    // +0x040
        uint32_t    flags;          // +0x060
        char        _pad2[0x38];    // +0x064
        uint8_t*    buckets;        // +0x09C
        uint32_t    bucketCount;    // +0x0A0

        // enumerates every component on this object; fn(uint32_t crc, void* component)
        template <class F>
        void forEachComponent(F&& fn) const
        {
            if (!buckets || bucketCount == 0 || bucketCount > 4096) return;
            for (uint32_t i = 0; i < bucketCount; i++)
            {
                auto* head = reinterpret_cast<ComponentNode*>(buckets + i * 8);
                int guard = 0;
                for (auto* n = head->next; n && n != head && guard < 64; n = n->next, guard++)
                    fn(n->crc, n->component);
            }
        }

        [[nodiscard]] void* findComponent(uint32_t crc) const
        {
            if (!buckets || bucketCount == 0) return nullptr;
            auto* head = reinterpret_cast<ComponentNode*>(buckets + (crc % bucketCount) * 8);
            for (auto* n = head->next; n && n != head; n = n->next)
                if (n->crc == crc) return n->component;
            return nullptr;
        }
    };
    static_assert(offsetof(EntityObject, position) == 0x034);
    static_assert(offsetof(EntityObject, flags) == 0x060);
    static_assert(offsetof(EntityObject, buckets) == 0x09C);
    static_assert(offsetof(EntityObject, bucketCount) == 0x0A0);

    static constexpr uint32_t kCrcAirMotion = 0xD8237C05;
} // namespace ssa::Game
#pragma pack(pop)