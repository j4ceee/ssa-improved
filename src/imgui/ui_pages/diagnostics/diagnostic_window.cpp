#include <imgui.h>
#include "addresses.h"
#include "game/custom/difficulty.h"
#include "securom/constant_lookup.h"
#include "game/game.h"
#include "game/magic_item_manager.h"
#include "game/mp_game.h"
#include "game/sound_system.h"
#include "game/world.h"
#include "game/data/levels.h"
#include "imgui/ui.h"
#include "imgui/ui_pages/diagnostics/debug_table.h"
#include "imgui/fonts/IconsMaterialDesign.h"
#include "window/state_cache.h"
#include "window/async_readback.h"
#include "window/texture_mods.h"

namespace ssa::UIPages
{
    void RenderLevelList(const Game::Game* game)
    {
        const auto currentLevelId = game->m_CurrLevel;

        DebugTable table{ "CRC32", "Next Level", "Level Name", "Level File", "Category", "Death Height" };
        for (const auto& desc : game->m_GamePackage.m_Levels)
        {
            DebugTable::Row& row = table.AddRow();
            if (desc.m_Id == currentLevelId)
                row.background = TableColor::RowHighlight;

            row.Add(Hex(desc.m_Id)).Add(Hex(desc.m_NextLevel));
            if (auto* name = Game::Data::GetLevelDisplayInfo(desc.m_Id))
                row.Add(name->displayName);
            else
                row.Dim("-");
            row.Add(desc.m_LevelFile.c_str()).Add(desc.m_Category.c_str()).Add(Fmt("%.1f", desc.m_fDeathHeight));
        }
        table.Show("##level_desc_list", ImGuiTableFlags_ScrollY);
    }

    void RenderCharacterList(const Game::Game* game)
    {
        DebugTable table{ "CRC32", "Name", "Name String ID", "File", "Attributes", "Scale" };
        for (const auto& desc : game->m_GamePackage.m_Characters)
        {
            table.AddRow()
                .Add(Hex(desc.m_Id))
                .Add(desc.m_Name.c_str())
                .Add(Hex(desc.m_NameStrId))
                .Add(desc.m_FilePath.c_str())
                .Add(Hex(desc.m_characterAttributes))
                .Add(Fmt("%.1f", desc.m_fScale));
        }
        table.Show("##char_desc_list", ImGuiTableFlags_ScrollY);
    }

    void RenderMagicItemList(const Game::Game* game)
    {
        auto* item = Game::MagicItemManager::instance()->currItem();

        DebugTable table{ "CRC32", "Name", "Type" };
        for (const auto& desc : game->m_GamePackage.m_MagicItems)
        {
            DebugTable::Row& row = table.AddRow();
            const bool isCurrent = item && item->m_Id == desc.m_Id;
            if (isCurrent)
                row.background = TableColor::RowHighlight;

            row.Add(Hex(desc.m_Id))
               .Add(isCurrent ? item->name() : desc.m_Name.c_str())
               .Add(Hex(desc.m_Type));
        }
        table.Show("##magic_item_list");
    }

    void RenderCurrentLevelStreams(const Game::Game* game)
    {
        auto* currLevelDesc = game->getCurrLevelDesc();
        if (!currLevelDesc)
        {
            ImGui::TextDisabled("No current level");
            return;
        }
        const auto& streamFiles = currLevelDesc->m_StreamFiles;
        const auto& stageFiles = currLevelDesc->m_StageFiles;

        if (!streamFiles.empty())
        {
            DebugTable table{ "Name" };
            for (const auto& file : streamFiles)
                table.AddRow().Add(file.c_str());
            table.Show("##stream_files");
        }
        else if (!stageFiles.empty())
        {
            DebugTable table{ "Stage", "Name" };
            int32_t stage = -1;
            for (const auto& stageFile : stageFiles)
            {
                for (const auto& file : stageFile.streamFiles)
                {
                    // the stage is only shown on its first row, but every row of the copy has it
                    const bool firstOfStage = stage != stageFile.state;
                    stage = stageFile.state;
                    table.AddRow()
                        .AddFull(firstOfStage ? Fmt("%d", stage) : std::string(), Fmt("%d", stage))
                        .Add(file.c_str());
                }
            }
            table.Show("##stage_files");
        }
        else
        {
            ImGui::TextDisabled("No stream or stage files.");
        }
    }

    void RenderStateCacheDiagnostics()
    {
        if (!ImGui::TreeNode("D3D State Cache"))
            return;
        ImGui::Indent();

        ImGui::Checkbox("Drop redundant state calls", &StateCache::g_enabled);
        ImGui::SameLine();
        UI::HelpMarker("Skips SetRenderState / SetSamplerState / shader constant calls that set the value the device already has.\n"
            "Turn it off to compare FPS or if something renders wrong.");

        uint32_t seen = 0, filtered = 0;
        for (int i = 0; i < StateCache::KindCount; ++i)
        {
            const StateCache::Counter& c = StateCache::g_last[i];
            seen += c.seen;
            filtered += c.filtered;
            ImGui::TextDisabled("%s: %u calls, %u dropped (%.0f%%)",
                                StateCache::KindName(static_cast<StateCache::Kind>(i)),
                                c.seen, c.filtered, c.seen > 0 ? 100.0 * c.filtered / c.seen : 0.0);
        }
        ImGui::TextDisabled("Total: %u calls, %u dropped (%.0f%%)", seen, filtered,
                            seen > 0 ? 100.0 * filtered / seen : 0.0);

        ImGui::Unindent();
        ImGui::TreePop();
    }

    void RenderConstantLookupDiagnostics()
    {
        if (!ImGui::TreeNode("Fast Constant Lookup (SecuROM)"))
            return;
        ImGui::Indent();

        namespace CL = Game::ConstantLookup;
        if (!CL::g_available)
        {
            ImGui::TextDisabled("Not available: the thunk or the pointer slot is not as expected (see the log)");
        }
        else
        {
            ImGui::Checkbox("Replace the game's constant lookup", &CL::g_enabled);
            ImGui::SameLine();
            UI::HelpMarker("The game's shader constant lookup runs through SecuROM's obfuscated code and takes microseconds per call. "
                "This points its pointer slot at an equivalent function of the mod.");
            ImGui::Checkbox("Verify against the game's function", &CL::g_verify);
            ImGui::SameLine();
            UI::HelpMarker(
                "Runs the game's own function as well and compares. On a difference the game's result is used. Slower, for testing only.");
            if (CL::g_verify)
                ImGui::TextDisabled("Compared: %u, mismatches: %u", CL::g_verified.load(), CL::g_mismatches.load());
            ImGui::TextDisabled("Slot 0x%08X -> %p (game function: %p)", static_cast<unsigned>(CL::Slot()),
                                *reinterpret_cast<void* const*>(CL::Slot()), CL::g_original);
        }

        ImGui::Unindent();
        ImGui::TreePop();
    }

    void RenderAsyncReadbackDiagnostics()
    {
        if (!ImGui::TreeNode("Asynchronous 4x4 Readback"))
            return;
        ImGui::Indent();

        ImGui::Checkbox("Never wait for the 4x4 readback", &AsyncReadback::g_enabled);
        ImGui::SameLine();
        UI::HelpMarker("Every frame the game reads a 4x4 copy of the scene back to the CPU and waits for it, which stalls the CPU. "
            "When on, the game gets the previous copy instead (one frame old), picked up before the next one is queued.\n"
            "Turn it off to compare FPS.");
        ImGui::TextDisabled("Game locks: %u answered from the early pickup, %u let through", AsyncReadback::g_deferred, AsyncReadback::g_blocked);
        ImGui::TextDisabled("Copies: %u picked up early, %u not ready", AsyncReadback::g_polled, AsyncReadback::g_missed);

        ImGui::Unindent();
        ImGui::TreePop();
    }

    void RenderDiagnosticWindow()
    {
        RenderConstantLookupDiagnostics();
        RenderStateCacheDiagnostics();
        RenderAsyncReadbackDiagnostics();

        if (ImGui::TreeNode("Game"))
        {
            ImGui::Indent();
            auto* game = Game::Game::instance();
            ImGui::TextDisabled("Instance: 0x%08X", static_cast<uint32_t>(reinterpret_cast<uintptr_t>(game)));
            ImGui::TextDisabled("Current Level: 0x%08X", game->m_CurrLevel);
            ImGui::TextDisabled("Paused: %d", game->m_bGameFrozen);
            ImGui::TextDisabled("Death Height: %.1f", game->m_fDeathHeight);
            ImGui::TextDisabled("Player Idle Time: %.1f", game->m_fPlayerIdleTime);
            ImGui::TextDisabled("Preparing next level: %d", game->m_bPrepareNextLevel);
            ImGui::TextDisabled("Game speed: %.1f", game->gameSpeeds.currRatio);

            if (ImGui::TreeNode("Level List"))
            {
                ImGui::Indent();
                RenderLevelList(game);
                ImGui::Unindent();
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Skylander List"))
            {
                ImGui::Indent();
                RenderCharacterList(game);
                ImGui::Unindent();
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Magic Item List"))
            {
                ImGui::Indent();
                RenderMagicItemList(game);
                ImGui::Unindent();
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Current Level Streams"))
            {
                ImGui::Indent();
                RenderCurrentLevelStreams(game);
                ImGui::Unindent();
                ImGui::TreePop();
            }

            ImGui::Unindent();
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("World"))
        {
            ImGui::Indent();
            auto* world = Game::World::instance();
            ImGui::TextDisabled("Instance: 0x%08X", static_cast<uint32_t>(reinterpret_cast<uintptr_t>(world)));
            ImGui::TextDisabled("Time: %.4f", world->time);
            ImGui::TextDisabled("Update Speed: %.4f", world->updateSpd);
            ImGui::TextDisabled("Playback Ratio: %.1f", world->playbackRatioNew);
            ImGui::TextDisabled("Frame Corrector: %.4f", world->frameCorrector);

            ImGui::TextDisabled("Active Views: 0x%08X", world->cameraSet.activeViews);
            ImGui::TextDisabled("Max Views: 0x%08X", world->cameraSet.maxViews);
            ImGui::TextDisabled("Primary CameraViewInterface:");
            if (auto* gv = world->cameraSet.primaryInterface())
            {
                ImGui::Indent();
                ImGui::TextDisabled("Instance: 0x%08X", static_cast<uint32_t>(reinterpret_cast<uintptr_t>(gv)));
                ImGui::TextDisabled("FOV: %.4f", gv->fov);
                ImGui::TextDisabled("Near Plane: %.4f", gv->nearPlane);
                ImGui::TextDisabled("Far Plane: %.4f", gv->farPlane);
                ImGui::TextDisabled("cameraMat pos: (%.1f, %.1f, %.1f)", gv->cameraMat.w.x, gv->cameraMat.w.y, gv->cameraMat.w.z);
                ImGui::TextDisabled("viewMat pos:   (%.1f, %.1f, %.1f)", gv->viewMat.w.x, gv->viewMat.w.y, gv->viewMat.w.z);
                ImGui::TextDisabled("invView pos:   (%.1f, %.1f, %.1f)", gv->invView.w.x, gv->invView.w.y, gv->invView.w.z);

                ImGui::TextDisabled("Camera Instance:");
                    ImGui::Indent();
                        auto* cameraInstance = gv->m_pCamera;
                        ImGui::TextDisabled("Instance: 0x%08X", static_cast<uint32_t>(reinterpret_cast<uintptr_t>(cameraInstance)));
                        ImGui::TextDisabled("FOV: %.4f", cameraInstance->m_FOV);
                    ImGui::Unindent();
                ImGui::Unindent();
            }

            ImGui::Unindent();
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Sound System"))
        {
            ImGui::Indent();
            auto* ss = Game::SoundSystem::instance();
            ImGui::TextDisabled("Instance: 0x%08X", static_cast<uint32_t>(reinterpret_cast<uintptr_t>(ss)));
            ImGui::TextDisabled("Music State: 0x%08X", Game::SoundManager::instance()->musicStateId);

            DebugTable table{ "#", "Event", "Game Object", "Playing ID" };
            int index = 0;
            for (const auto& s : ss->playing)
            {
                if (++index > 256) break; // corrupted-list guard

                DebugTable::Row& row = table.AddRow();
                row.Add(std::to_string(index));
                if (!s.ptr) continue;
                row.Add(Hex(s.ptr->eventId)).Add(Hex(s.ptr->gameObject)).Add(Hex(s.ptr->playingId));
            }
            table.Show("##playing_sounds");

            ImGui::Unindent();
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("MP Game"))
        {
            ImGui::Indent();
            auto* mpGame = Game::MPGame::instance();
            ImGui::TextDisabled("Instance: 0x%08X", static_cast<uint32_t>(reinterpret_cast<uintptr_t>(mpGame)));
            ImGui::TextDisabled("State: %d", mpGame->m_bIsPvPMatch);
            ImGui::TextDisabled("Rules: 0x%08X", static_cast<uint32_t>(reinterpret_cast<uintptr_t>(mpGame->m_pRules)));

            if (ImGui::Button("Toggle PvP Match"))
            {
                mpGame->SetInPvPMatch(!mpGame->isInPvPMatch());
            }
            ImGui::SameLine();
            UI::HelpMarker("Put one player in enemy team for melee attacks to register while PvP is active");

            ImGui::Unindent();
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Magic Item Manager"))
        {
            ImGui::Indent();
            auto* mgr = Game::MagicItemManager::instance();
            ImGui::TextDisabled("Instance: 0x%08X", static_cast<uint32_t>(reinterpret_cast<uintptr_t>(mgr)));
            ImGui::TextDisabled("Current Item:");
            ImGui::Indent();
            if (auto* item = mgr->currItem())
            {
                ImGui::TextDisabled("Name: %s", item->name());
                ImGui::TextDisabled("ID: 0x%08X", item->m_Id);
                ImGui::TextDisabled("Item Type: 0x%08X", item->m_Type);
                ImGui::TextDisabled("Lifetime: %.2f", item->m_Lifetime);
                ImGui::TextDisabled("Timer: %.2f", item->m_Timer);
                ImGui::TextDisabled("Time Flag: %d", item->b_TimeFlag);
                ImGui::TextDisabled("Active: %d", item->b_Active);
                ImGui::TextDisabled("Persist: %d", item->b_Persist);
                ImGui::TextDisabled("Used: %d", item->b_Used);
                ImGui::TextDisabled("Hidden: %d", item->b_Hidden);
                ImGui::TextDisabled("Is Pet: %d", item->b_IsPet);
            }
            else
            {
                ImGui::TextDisabled("-");
            }
            ImGui::Unindent();

            ImGui::Unindent();
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Damage"))
        {
            ImGui::Indent();
            auto* scs = Game::SpyroCharacterSettings::instance();
            auto& enemy = scs->m_enemySettings;
            const bool levelLoaded = enemy.m_pLevelAttribute != nullptr;

            if (levelLoaded)
                ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.6f, 1.0f), ICON_MD_CHECK_CIRCLE " Level loaded");
            else
                ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.1f, 1.0f), ICON_MD_WARNING " Not in a level");

            ImGui::TextDisabled("Instance: 0x%08X", static_cast<uint32_t>(reinterpret_cast<uintptr_t>(scs)));
            ImGui::TextDisabled("LevelAttr 0x%08X", static_cast<uint32_t>(reinterpret_cast<uintptr_t>(enemy.m_pLevelAttribute)));
            ImGui::TextDisabled("Snapshot 0x%08X", static_cast<uint32_t>(reinterpret_cast<uintptr_t>(Game::Difficulty::s_snapshotAttr)));

            ImGui::Spacing();

            static constexpr const char* kClassNames[8] = {
                "0 - Swarmer", "1 - Spawner", "2 - Unknown", "3 - Unknown", "4 - Unknown", "5 - Unknown", "6 - Unknown", "7 - Unknown",
            };

            DebugTable table{ "Class", "DMG (raw)", "DMG (current)" };
            for (int i = 0; i < 8; i++)
            {
                DebugTable::Row& row = table.AddRow();
                row.Add(kClassNames[i]);
                if (levelLoaded)
                    row.Add(Fmt("%.2f", Game::Difficulty::s_baseDmgSnapshot[i])).Add(Fmt("%.2f", enemy.m_fBaseDamage[i]));
                else
                    row.Dim("--").Dim("--");
            }
            table.Show("##enemy_stats");

            ImGui::Unindent();
            ImGui::TreePop();
        }

        RenderAttackLightsDiagnostics();

        ImGui::Spacing();
        ImGui::Spacing();

        if (ImGui::Button("Open Character Table"))
            UI::Get()->SetCharacterTableVisible(true);

        ImGui::Spacing();

        if (ImGui::Button("Open Target Table"))
            UI::Get()->SetTargetTableVisible(true);

        ImGui::Spacing();

        if (ImGui::Button("Open Render Target Table"))
            UI::Get()->SetRenderTargetTableVisible(true);
    }
} // namespace ssa::UIPages
