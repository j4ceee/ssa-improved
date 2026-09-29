#pragma once

#include <cmath>
#include <meow_hook/util.h>
#include "game/character.h"
#include "game/entity_object.h"
#include "game/pad_state.h"
#include "config.h"
#include "imgui/ui.h"

namespace ssa::Game::Jump
{
    static constexpr uint32_t kJumpRawButton = 0x100; // left stick

    inline bool TryJump(Character* ch, float height)
    {
        auto* mc = ch->m_pMotionControl;
        auto* obj = static_cast<EntityObject*>(ch->m_pObject);
        if (!mc || !obj || mc->airMotionSuspended) return false;
        if (ch->charExtraBits & (kCharBitNoLaunch | kCharBitAirborne | kCharBitCinemaLock | kCharBitFreeze))
            return false;

        auto* phys = mc->physicsObject();
        auto* rb = phys ? phys->rigidBody() : nullptr;
        void* air = obj->findComponent(kCrcAirMotion);
        if (!rb || !air) return false;

        // GetJumpImpulse: v = sqrt(2|g|h) against gravity; keep horizontal momentum
        const Vec3 g = rb->gravityDir * rb->gravityMag;
        const float gLen = std::sqrt(g.x * g.x + g.y * g.y + g.z * g.z);
        if (gLen <= 0.0f) return false;
        const Vec3 up = g * (-1.0f / gLen);
        const Vec3 cur = rb->linearVelocity;
        const float curUp = cur.x * up.x + cur.y * up.y + cur.z * up.z;
        Vec3 vel = cur - up * curUp + up * std::sqrt(2.0f * gLen * height);

        rb->linearVelocity = vel; // SetLinearVelocity
        phys->releaseTraction(); // SetTraction(true, 0, 0)

        meow_hook::func_call<void>(reinterpret_cast<uintptr_t>(GetAddress(AIR_MOTION_JUMP)), static_cast<const Vec3*>(&vel), air);
        return true;
    }

    inline void Update()
    {
        const bool active = g_config.jumpEnabled && !UI::Get()->IsVisible();
        if (!active) return;

        auto* pads = PadState::array();

        for (const auto& ref : *Character::instanceSkylandersList())
        {
            auto* ch = ref.mPtr;
            if (!ch || !ch->isPlayer()) continue;

            const int id = ch->contID;
            if (id < 0 || id >= kPadCount) continue;

            if (pads[id].hitButtons & kJumpRawButton)
                TryJump(ch, (float)g_config.jumpHeight);
        }
    }
}
