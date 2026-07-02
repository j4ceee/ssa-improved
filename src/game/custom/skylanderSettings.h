#pragma once

#include "game/character.h"
#include "config.h"

namespace ssa::Game::SkylanderSettings
{
    // called from hook_Present every frame
    inline void Update()
    {
        auto* list = Character::instanceSkylandersList();

        for (const auto& ref : *list)
        {
            auto* ch = ref.mPtr;

            if (!ch)
                continue;

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
    }
}
