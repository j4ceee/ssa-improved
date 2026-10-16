#pragma once

#include "game/cutscene_triggers.h"
#include "game/shared_ptr.h"
#include "game/ui_game.h"
#include "game/world.h"

namespace ssa::Game::CinemaSkip
{

    // first playthrough: bSeenBefore == 0 -> scene can't be skipped
    // marking every live trigger as "seen" & not dev locked makes everything skippable:
    // - bSeenBefore = 1 -> skippable on first playthrough (wii behaviour)
    // - bNeverSkip = 0 -> ignore dev locks
    inline void UpdateCinemaTriggers()
    {
        auto* list = CinemaTrigger::instanceList();
        if (!list || list->empty()) return;

        for (CinemaTrigger* trigger : *list)
        {
            if (!trigger) continue;
            trigger->bSeenBefore = 1;
            trigger->bNeverSkip = 0;
        }
    }

    // Camera shots and script cinemas are not CinemaTriggers: they only live in the world's update list while they run
    inline void UpdateRunningScenes()
    {
        World* world = World::instance();
        if (world->componentList.empty()) return;

        const void* cameraAttach = GetAddress(CAMERA_ATTACH_TRIGGER_VTABLE);
        const void* closeUp = GetAddress(SPYRO_CLOSEUP_TRIGGER_VTABLE);
        const void* scriptCinema = GetAddress(SCRIPT_CINEMA_VTABLE);

        for (const WeakPtr<Component>& ref : world->componentList)
        {
            if (!Shared::IsAlive(ref)) continue; // the list holds weak references

            Component* component = ref.ptr;
            if (component->vtable == cameraAttach || component->vtable == closeUp)
            {
                static_cast<CameraShotTrigger*>(component)->playerSkip = 1;
            }
            else if (component->vtable == scriptCinema)
            {
                auto* cinema = static_cast<ScriptCinema*>(component);
                if (cinema->mode == ScriptCinema::kSkippableMode) cinema->playerSkip = 1; // Update only looks at it in this mode
            }
        }
    }

    // UITextDialog::Open: bInteractive == 0 for voiced dialogues -> can't be dismissed
    // - setting bInteractive = 1 after Open puts them on the game's own dismiss path (1st press finishes the text, 2nd press sets done -> Kill stops the voice, fires triggers)
    inline void UpdateDialogue()
    {
        auto* ui = UIGame::instance();
        if (!ui || ui->dialogs.empty()) return;

        UIDialog* dlg = *ui->dialogs.begin(); // UpdateDialog only runs the first one
        if (!dlg || !dlg->isTextDialog()) return;

        if (dlg->type == 2 || dlg->type == 4 || dlg->type == 5) return; // question dialogues need an answer
        if (dlg->voiceOnly) return; // plays over gameplay, blocks nothing
        if (!dlg->bPause && !dlg->bLockControl) return; // doesn't take over input

        dlg->bInteractive = 1;
    }

    inline void Update()
    {
        UpdateCinemaTriggers();
        UpdateRunningScenes();
        UpdateDialogue();
    }
}
