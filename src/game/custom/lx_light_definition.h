#pragma once

#include <cstddef>
#include <cstdint>
#include <iterator>
#include "game/light.h"

namespace ssa::Game
{
    // Light definition in the engine's LX format (type descriptors + values, linked with relative offsets)
    // built in mod-owned memory, so Light::createFromLX can create a light without a model file.
    // Layout: pointer -> record (struct of 13 named members) -> member types (int / float / float[4]).
    // Member keys are CRCs the Light parser looks up. All lights made from it are omni lights (type 4).
    // The Browser points into this object, so it has to stay at a fixed address while the factory runs.
    struct LxLightDefinition
    {
        struct Member
        {
            uint32_t key; // name CRC
            int32_t type; // type descriptor, relative to structType
            int32_t value; // offset of the value inside Values
        };

        struct Values
        {
            int32_t type = 4;
            float colour[4] = {0.35f, 0.8f, 1.5f, 1.0f};
            float colour2[4] = {};
            float range = 6.0f, range2 = 0, attenuate = 0, attenuate2 = 0;
            float hotspot = 0, falloff = 0, offset = 0;
            int32_t diffuse = 1, specular = 1, ambient = 0;
        };

        uint32_t pointerType = 10;
        int32_t pointerValue = 0; // relative to itself
        uint32_t recordName = 0;
        int32_t recordType = 0; // relative to recordName
        Values values;
        uint32_t structType = 6;
        uint32_t memberCount = 13;
        Member members[13] = {};
        uint32_t intType = 0;
        uint32_t floatType = 1;
        uint32_t vectorType = 8;
        uint32_t vectorCount = 4;
        uint32_t vectorStride = 4;
        int32_t vectorElement = 0; // relative to vectorType

        LxLightDefinition()
        {
            enum class Kind { Int, Float, Vector4 };
            struct FieldDesc
            {
                uint32_t key;
                size_t offset;
                Kind kind;
            };

            constexpr FieldDesc fields[] = {
                {0x9884ece8, offsetof(Values, type), Kind::Int}, // lightType
                {0xfaf865ce, offsetof(Values, colour), Kind::Vector4}, // colour
                {0x66f3a9df, offsetof(Values, colour2), Kind::Vector4}, // colour2
                {0x93875a49, offsetof(Values, range), Kind::Float}, // range
                {0x1546c063, offsetof(Values, range2), Kind::Float}, // range2
                {0x80686542, offsetof(Values, attenuate), Kind::Float}, // attenuate
                {0x8287f6d4, offsetof(Values, attenuate2), Kind::Float}, // attenuate2
                {0x48b38313, offsetof(Values, hotspot), Kind::Float}, // hotspot
                {0x4f6d2cf8, offsetof(Values, falloff), Kind::Float}, // falloff
                {0x590acad0, offsetof(Values, offset), Kind::Float}, // offset
                {0x25550d66, offsetof(Values, diffuse), Kind::Int}, // affectDiffuse
                {0x71a75345, offsetof(Values, specular), Kind::Int}, // affectSpecular
                {0xe5786d73, offsetof(Values, ambient), Kind::Int}, // ambientOnly
            };
            static_assert(std::size(fields) == 13);

            pointerValue = relative(&values, &pointerValue);
            recordType = relative(&structType, &recordName);
            vectorElement = relative(&floatType, &vectorType);

            for (size_t i = 0; i < std::size(fields); ++i)
            {
                const FieldDesc& f = fields[i];
                const void* type = f.kind == Kind::Int ? &intType : f.kind == Kind::Float ? &floatType : &vectorType;
                members[i] = {f.key, relative(type, &structType), static_cast<int32_t>(f.offset)};
            }
        }

        LxLightDefinition(const LxLightDefinition&) = delete;
        LxLightDefinition& operator=(const LxLightDefinition&) = delete;

        // colour is linear HDR RGB, range in game world units
        void set(const float rgb[3], const float range)
        {
            for (size_t i = 0; i < 3; ++i)
                values.colour[i] = rgb[i];
            values.range = range;
        }

        [[nodiscard]] LxBrowser browser() const { return {&pointerType, &pointerValue}; }

    private:
        static int32_t relative(const void* to, const void* from)
        {
            return static_cast<int32_t>(reinterpret_cast<uintptr_t>(to) - reinterpret_cast<uintptr_t>(from));
        }
    };

    static_assert(offsetof(LxLightDefinition, values) == offsetof(LxLightDefinition, recordName) + 8);
    static_assert(offsetof(LxLightDefinition, members) == offsetof(LxLightDefinition, structType) + 8);
} // namespace ssa::Game
