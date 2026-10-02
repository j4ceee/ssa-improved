#pragma once

#include "game/character.h"
#include "config.h"

namespace ssa::Game::SkylanderSettings
{
    // player-only invisible walls: only players receive this collision layer, so stripping it
    // lets them pass while floors / regular walls (other layers) stay solid
    inline void UpdatePlayerBlockers(Character* ch, const bool remove, const bool wasRemoved)
    {
        if (!ch->isPlayer())
            return;

        auto* body = ch->physicsBody();
        if (!body) return;

        if (remove)
        {
            body->maskReceive = static_cast<uint16_t>(body->maskReceive & ~kColLayerPlayerBlocker);
        }
        else if (wasRemoved)
        {
            // option was just turned off: restore the layer once (only if the game had it set)
            body->maskReceive = static_cast<uint16_t>(body->maskReceive | (body->maskReceiveOriginal & kColLayerPlayerBlocker));
        }
    }

    // MotionControl::PostPostUpdate
    // - snaps players back to their previous XY position (and zeroes their horizontal velocity) whenever a ray straight down finds no valid ground: level edges, disconnected turtles, water / lava
    // - m_checkMove gates the whole check; constructor sets it to 1 and states that clear it temporarily (jump pads etc.) restore it themselves
    inline void UpdateEdgeGuards(Character* ch, const bool remove, const bool wasRemoved)
    {
        if (!ch->isPlayer())
            return;

        auto* mc = ch->m_pMotionControl;
        if (!mc) return;

        if (remove)
            mc->checkMove = 0;
        else if (wasRemoved)
            mc->checkMove = 1; // option was just turned off: back to the constructor default
    }

    // called from hook_Present every frame
    inline void Update()
    {
        auto* list = Character::instanceSkylandersList();

        static bool s_blockersRemoved = false;
        const bool removeBlockers = g_config.removePlayerBlockers;
        static bool s_edgeGuardsRemoved = false;
        const bool removeEdgeGuards = g_config.removeEdgeGuards;

        for (const auto& ref : *list)
        {
            auto* ch = ref.mPtr;

            if (!ch)
                continue;

            UpdatePlayerBlockers(ch, removeBlockers, s_blockersRemoved);
            UpdateEdgeGuards(ch, removeEdgeGuards, s_edgeGuardsRemoved);

            if (ch->isPlayer1())
            {
                ch->setGodMode(g_config.p1GodMode);
                ch->setIgnoreKnockback(g_config.p1NoKnockback);
                ch->setIgnoreHitReaction(g_config.p1NoHitReaction);
            }
            else
            {
                ch->setGodMode(g_config.p2GodMode);
                ch->setIgnoreKnockback(g_config.p2NoKnockback);
                ch->setIgnoreHitReaction(g_config.p2NoHitReaction);
            }
        }

        s_blockersRemoved = removeBlockers;
        s_edgeGuardsRemoved = removeEdgeGuards;
    }
}
