#pragma once

#include <cmath>
#include <cstdint>
#include "addresses.h"
#include "config.h"
#include "log.h"
#include "game/character.h"
#include "game/entity_object.h"
#include "game/game.h"
#include "game/shared_ptr.h"

// Skylander movement:
// - Unrestricted Movement: walk off ledges and level edges, jump gaps and still get recovered
// - Unlimited co-op distance: players can move as far apart as they like
// - Rejoin co-op players: P2 is brought back to P1 when they are far apart (needs one of the other two, otherwise the game does it itself)
namespace ssa::Game::Movement
{
    // ----------------------------------------------------------------------------------------------------------------
    // tuning
    // ----------------------------------------------------------------------------------------------------------------
    inline constexpr float kRejoinDistance = 60.0f; // XY distance that makes P2 rejoin P1 while co-op distance is unlimited
    inline constexpr float kRejoinDistanceVanilla = 26.5f; // vanilla: max distance 26 + 0.5
    inline constexpr float kRejoinDelay = 0.5f; // seconds the players have to stay that far apart
    inline constexpr uint64_t kRejoinCooldownMs = 3000;
    inline constexpr float kFallTimerScale = 2.0f; // 1 = vanilla (StateDrown after 0.5s without ground), 2 = after 1s
    inline constexpr float kWaterDrownDelay = 0.2f; // seconds in water / lava before StateDrown is forced (0.2 = vanilla)
    inline constexpr float kLeashDisabledMax = 1.0e6f; // max co-op distance while the option is on
    inline constexpr float kLeashVanillaMax = 26.0f; // only used when the vanilla value was never read
    inline constexpr uint64_t kGroundedMs = 300; // a position counts as safe after this long on the ground
    inline constexpr uint64_t kSampleMinAgeMs = 750; // fallback respawn uses a sample at least this old, which steps back from the ledge just left
    inline constexpr float kSampleSpacing = 3.0f; // minimum XY distance between two samples
    inline constexpr float kPulseSpacing = 4.0f; // XY distance walked on safe ground between two checkMove pulses
    inline constexpr float kExitMovedMin = 4.0f; // after StateDrown the game must have moved the player at least this far, otherwise it found no respawn point
    inline constexpr uint64_t kForceCooldownMs = 1000; // minimum time between two ForceState calls on one player

    // ----------------------------------------------------------------------------------------------------------------
    // game constants
    // ----------------------------------------------------------------------------------------------------------------

    inline constexpr uint32_t kCharBitStateLock = 0x40000000; // set by StateDrown::Enter and StateWaitJoin::Enter, cleared by their Exit

    // crc32 hashes of the state names
    inline constexpr uint32_t kForceDrown = 0xEA5EB72D;
    inline constexpr uint32_t kForceWaitJoin = 0xE23ED288;

    // ----------------------------------------------------------------------------------------------------------------
    // game calls
    // ----------------------------------------------------------------------------------------------------------------

    // respawn points the game keeps, only the ones still alive
    inline int CountRespawnPoints()
    {
        int alive = 0;
        int visited = 0;
        auto* list = reinterpret_cast<List<WeakPtr<void>>*>(GetAddress(RESPAWN_POINT_LIST));
        for (auto it = list->begin(); it != list->end(); ++it)
        {
            if (++visited > 256) break; // guard against a wrong address
            if (Shared::IsAlive(*it)) alive++;
        }
        return alive;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // per player bookkeeping
    // ----------------------------------------------------------------------------------------------------------------
    struct Sample
    {
        Vec3 pos;
        uint64_t timeMs;
    };

    struct PlayerTrack
    {
        static constexpr int kRing = 16;

        Sample ring[kRing] = {}; // last safe positions
        int head = 0; // next write index
        int count = 0;

        uint64_t groundedSinceMs = 0;
        uint64_t lastForceMs = 0;
        float waterTime = 0.0f;
        float prevInvalidGround = 0.0f;

        bool overrideApplied = false; // we wrote checkMove = 0 and still owe the game a 1
        Vec3 lastPulsePos = {};
        bool hasPulsePos = false;

        bool respawning = false; // StateDrown or StateWaitJoin is running
        bool rejoining = false; // ... and it is StateWaitJoin
        Vec3 respawnStart = {}; // where it began, to see whether the game moved the player when it ended

        void reset()
        {
            head = 0;
            count = 0;
            groundedSinceMs = 0;
            waterTime = 0.0f;
            prevInvalidGround = 0.0f;
            hasPulsePos = false;
            respawning = false;
        }

        [[nodiscard]] const Sample* newest() const
        {
            return count > 0 ? &ring[(head - 1 + kRing) % kRing] : nullptr;
        }

        void push(const Vec3& pos, uint64_t nowMs)
        {
            if (const Sample* last = newest())
            {
                const float dx = pos.x - last->pos.x;
                const float dy = pos.y - last->pos.y;
                if (dx * dx + dy * dy < kSampleSpacing * kSampleSpacing)
                    return;
            }

            ring[head] = {pos, nowMs};
            head = (head + 1) % kRing;
            if (count < kRing) count++;
        }

        // newest sample that is old enough, otherwise the oldest one we have
        [[nodiscard]] const Sample* pick(uint64_t nowMs) const
        {
            const Sample* oldest = nullptr;
            for (int i = 0; i < count; i++)
            {
                const Sample* s = &ring[(head - 1 - i + kRing) % kRing];
                oldest = s;
                if (nowMs - s->timeMs >= kSampleMinAgeMs)
                    return s;
            }
            return oldest;
        }
    };

    inline PlayerTrack s_tracks[2]; // indexed by Character::contID

    // ----------------------------------------------------------------------------------------------------------------
    // leash
    // ----------------------------------------------------------------------------------------------------------------

    // soft pull in PostUpdate scales with (dist - min) / (max - min), a huge max makes it vanish. min stays untouched
    inline void UpdateLeash(bool active)
    {
        static bool s_captured = false;
        static float s_vanillaMax = kLeashVanillaMax;

        auto* maxDist = static_cast<float*>(GetAddress(PLAYER_LEASH_MAX));

        if (active)
        {
            if (!s_captured && *maxDist != kLeashDisabledMax)
            {
                s_vanillaMax = *maxDist;
                s_captured = true;
            }
            // Tune::Tune may run again on a level load, so this is written every frame
            *maxDist = kLeashDisabledMax;
        }
        else if (*maxDist == kLeashDisabledMax)
        {
            *maxDist = s_vanillaMax;
        }
    }

    // ----------------------------------------------------------------------------------------------------------------
    // per player
    // ----------------------------------------------------------------------------------------------------------------

    // StateDrown or StateWaitJoin just started. The game moves the player when it ends, so only remember where we were
    inline void OnRespawnBegan(int slot, const Character* ch, PlayerTrack& track)
    {
        const auto* obj = static_cast<EntityObject*>(ch->m_pObject);
        track.respawnStart = obj->position;
        track.rejoining = ch->isInState(STATE_WAIT_JOIN_VTABLE);

        LogDebug("[Movement] P%d %s at (%.1f, %.1f, %.1f), the game knows %d respawn points",
                 slot + 1, track.rejoining ? "rejoining" : "drowning", obj->position.x, obj->position.y,
                 obj->position.z, CountRespawnPoints());
    }

    // state ended. Normally the game has moved the player to a respawn point (or next to the partner) by now.
    // If it did not, no usable point existed & player is still where they fell -> use our last safe position
    inline void OnRespawnEnded(int slot, const Character* ch, MotionControl* mc, const PlayerTrack& track, uint64_t nowMs)
    {
        const auto* obj = static_cast<EntityObject*>(ch->m_pObject);
        const Vec3 d = obj->position - track.respawnStart;
        const float moved = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);

        // rejoin has its own destination next to the partner, never second-guess it
        if (track.rejoining)
        {
            LogDebug("[Movement] P%d rejoined at (%.1f, %.1f, %.1f), %.1f units from where it was", slot + 1,
                     obj->position.x, obj->position.y, obj->position.z, moved);
            return;
        }

        if (moved >= kExitMovedMin)
        {
            LogDebug("[Movement] P%d respawned by the game at (%.1f, %.1f, %.1f), %.1f units from where it drowned",
                     slot + 1, obj->position.x, obj->position.y, obj->position.z, moved);
            return;
        }

        const Sample* sample = track.pick(nowMs);
        if (!sample)
        {
            LogDebug("[Movement] P%d was not moved by the game and no safe position is known", slot + 1);
            return;
        }

        Vec3 dir = mc->aimDir;
        const float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
        dir = len > 1.0e-3f ? dir * (1.0f / len) : Vec3{0.0f, 1.0f, 0.0f};

        LogDebug("[Movement] P%d was not moved by the game (moved %.1f), using safe position (%.1f, %.1f, %.1f), sample age %llu ms",
            slot + 1, moved, sample->pos.x, sample->pos.y, sample->pos.z,
            static_cast<unsigned long long>(nowMs - sample->timeMs));
        mc->teleport(sample->pos, dir);
    }

    // MotionControl::Update forces StateDrown once the time without ground passes 0.5. Stretch that time
    inline void StretchFallTimer(MotionControl* mc, PlayerTrack& track)
    {
        float& t = mc->invalidGroundTime;
        if (kFallTimerScale > 1.0f && t > track.prevInvalidGround && t - track.prevInvalidGround < 1.0f)
            t = track.prevInvalidGround + (t - track.prevInvalidGround) / kFallTimerScale;
        track.prevInvalidGround = t; // game resets it to 0 itself when the ground query has a hit again
    }

    // safe positions for our fallback & checkMove pulses that keep the game's own respawn points coming
    inline void TrackGround(const Character* ch, MotionControl* mc, const EntityObject* obj, PlayerTrack& track, uint64_t nowMs)
    {
        const bool grounded = mc->onGround && !mc->airMotion && !mc->flying && !mc->hovering && !mc->alwaysHovering
            && !mc->onWater && !mc->onLava && !(ch->charExtraBits & kCharBitAirborne);

        if (!grounded)
        {
            track.groundedSinceMs = 0;
            return;
        }

        if (!track.groundedSinceMs) track.groundedSinceMs = nowMs;
        if (nowMs - track.groundedSinceMs < kGroundedMs) return;

        track.push(obj->position, nowMs);

        // game only updates its respawn points in frames where checkMove != 0 (PlayerRespawnPoint::UpdateAll)
        // one such frame every few units on safe ground makes it drop points like it does without this option
        // if the player steps off a ledge in that very frame the game snaps them back once
        const float dx = obj->position.x - track.lastPulsePos.x;
        const float dy = obj->position.y - track.lastPulsePos.y;
        if (!track.hasPulsePos || dx * dx + dy * dy >= kPulseSpacing * kPulseSpacing)
        {
            track.lastPulsePos = obj->position;
            track.hasPulsePos = true;
            mc->checkMove = 1;
        }
    }

    // vanilla blocks non-swimmers at the shore (checkMove) or drowns them after 0.2 s in water / lava, we only keep the second part
    inline void UpdateWater(int slot, Character* ch, MotionControl* mc, const EntityObject* obj, PlayerTrack& track, uint64_t nowMs, float dt)
    {
        if (mc->airMotion || !(mc->onWater || mc->onLava) || mc->canMove(mc->onWater != 0, mc->onLava != 0))
        {
            track.waterTime = 0.0f;
            return;
        }

        track.waterTime += dt;
        if (track.waterTime < kWaterDrownDelay || nowMs - track.lastForceMs < kForceCooldownMs)
            return;

        LogDebug("[Movement] P%d in %s and cannot stay there, forcing Drown at (%.1f, %.1f, %.1f)",
                 slot + 1, mc->onLava ? "lava" : "water", obj->position.x, obj->position.y, obj->position.z);
        track.lastForceMs = nowMs;
        track.waterTime = 0.0f;
        ch->forceState(kForceDrown);
    }

    inline void UpdatePlayer(int slot, Character* ch, bool active, uint64_t nowMs, float dt)
    {
        auto* mc = ch->m_pMotionControl;
        auto* obj = static_cast<EntityObject*>(ch->m_pObject);
        if (!mc || !obj || obj->isDeleted()) return;

        PlayerTrack& track = s_tracks[slot];
        const bool locked = (ch->charExtraBits & kCharBitStateLock) != 0;

        if (!active)
        {
            // option off (or PvP): hand checkMove back as soon as no state is running that manages it itself
            if (track.overrideApplied && !locked)
            {
                mc->checkMove = 1;
                track.overrideApplied = false;
            }
            track.respawning = false;
            return;
        }

        mc->checkMove = 0;
        track.overrideApplied = true;

        const bool respawning = locked && (ch->isInState(STATE_DROWN_VTABLE) || ch->isInState(STATE_WAIT_JOIN_VTABLE));
        if (respawning && !track.respawning)
            OnRespawnBegan(slot, ch, track);
        else if (!respawning && track.respawning && !locked)
            OnRespawnEnded(slot, ch, mc, track, nowMs);
        track.respawning = respawning;

        // a state that manages the player or a cutscene / freeze that moves them
        if (locked || (ch->charExtraBits & (kCharBitCinemaLock | kCharBitFreeze)))
            return;

        StretchFallTimer(mc, track);
        TrackGround(ch, mc, obj, track, nowMs);
        UpdateWater(slot, ch, mc, obj, track, nowMs, dt);
    }

    // ----------------------------------------------------------------------------------------------------------------
    // rejoin
    // ----------------------------------------------------------------------------------------------------------------

    // vanilla pulls the second player to the first one at 26.5 units, that trigger is dead while checkMove is 0
    // trigger is ours (limit = XY distance, 0 = off), the state is vanilla
    inline void UpdateRejoin(const Character* p1, Character* p2, float limit, uint64_t nowMs, float dt)
    {
        static float s_apartTime = 0.0f;
        static uint64_t s_cooldownUntilMs = 0;

        // one of them is already drowning / rejoining, or a cutscene is running
        constexpr uint32_t kBusy = kCharBitStateLock | kCharBitCinemaLock | kCharBitFreeze;

        const auto* o1 = p1 ? static_cast<EntityObject*>(p1->m_pObject) : nullptr;
        const auto* o2 = p2 ? static_cast<EntityObject*>(p2->m_pObject) : nullptr;

        float dx = 0.0f, dy = 0.0f;
        if (o1 && o2 && !o1->isDeleted() && !o2->isDeleted())
        {
            dx = o2->position.x - o1->position.x;
            dy = o2->position.y - o1->position.y;
        }

        const bool apart = o1 && o2 && dx * dx + dy * dy > limit * limit;
        if (limit <= 0.0f || !apart || nowMs < s_cooldownUntilMs || ((p1->charExtraBits | p2->charExtraBits) & kBusy))
        {
            s_apartTime = 0.0f;
            return;
        }

        s_apartTime += dt;
        if (s_apartTime < kRejoinDelay) return;

        LogDebug("[Movement] players %.1f units apart (limit %.1f), P2 rejoins P1", std::sqrt(dx * dx + dy * dy), limit);

        s_apartTime = 0.0f;
        s_cooldownUntilMs = nowMs + kRejoinCooldownMs;
        p2->forceState(kForceWaitJoin);
    }

    // ----------------------------------------------------------------------------------------------------------------
    // called from hook_Present every frame
    // ----------------------------------------------------------------------------------------------------------------
    inline void Update()
    {
        const auto* game = Game::instance();

        // game does not simulate here, nothing to read or write
        if (game->isTransitioningLevel() || game->isFrozen())
            return;

        // PvP arenas keep the vanilla edge guards and leash
        const bool pvp = game->isInPvPCategory();
        const bool freeMovement = g_config.freeMovement && !pvp;
        const bool unlimitedDistance = g_config.unlimitedCoopDistance && !pvp;

        // as long as checkMove is 1 and the distance is limited the game brings the players together itself
        const bool rejoin = g_config.coopRejoin && !pvp && (freeMovement || unlimitedDistance);
        const float rejoinLimit = !rejoin ? 0.0f : (unlimitedDistance ? kRejoinDistance : kRejoinDistanceVanilla);

        static uint32_t s_level = 0;
        if (game->m_CurrLevel != s_level)
        {
            s_level = game->m_CurrLevel;
            s_tracks[0].reset();
            s_tracks[1].reset();
        }

        static uint64_t s_lastMs = 0;
        const uint64_t nowMs = GetTickCount64();
        float dt = s_lastMs ? static_cast<float>(nowMs - s_lastMs) / 1000.0f : 0.0f;
        s_lastMs = nowMs;
        if (dt > 0.25f) dt = 0.25f; // paused / hitch, do not let timers jump

        UpdateLeash(unlimitedDistance);

        Character* players[2] = {};
        for (const auto& ref : *Character::instanceSkylandersList())
        {
            auto* ch = ref.mPtr;
            if (!ch || !ch->isPlayer()) continue;

            const int slot = ch->contID;
            if (slot < 0 || slot > 1) continue;

            players[slot] = ch;
            UpdatePlayer(slot, ch, freeMovement, nowMs, dt);
        }

        UpdateRejoin(players[0], players[1], rejoinLimit, nowMs, dt);
    }
}
