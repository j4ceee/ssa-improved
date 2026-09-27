#pragma once

#include "game/cinema_trigger.h"

namespace ssa::Game::CinemaSkip
{
    // first playthrough: bSeenBefore == 0 -> scene can't be skipped
    // marking every live trigger as "seen" gives wii behaviour:
    // - dev locked scenes (bNeverSkip) stay unskippable
    // - the save's "watched" record is still written normally in UnTrigger (unlike forcing bAlwaysSkip)
    // - ReallyGo shows the skip prompt (EnableCinematicUI) as long as the flag is set before the scene starts
    inline void Update()
    {
        auto* list = CinemaTrigger::instanceList();
        if (!list || list->empty()) return;

        for (CinemaTrigger* trigger : *list)
        {
            if (trigger) trigger->bSeenBefore = 1;
        }
    }
}
