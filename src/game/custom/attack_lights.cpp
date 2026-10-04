#include "attack_lights.h"

#include <Windows.h>
#include <algorithm>
#include <iterator>
#include "config.h"
#include "log.h"
#include "game/character.h"
#include "game/entity_object.h"
#include "game/game.h"
#include "game/light.h"
#include "game/projectile.h"
#include "game/shared_ptr.h"
#include "game/world.h"
#include "game/custom/lx_light_definition.h"

/**
 * Strategy:
 * - find the projectiles of the local players in the world's component lists
 * - for every projectile with a profile, create a Light on the projectile's owner object through the game's light factory
 * - the mod holds one strong reference on each light and one weak reference on the projectile's control block, so neither can be reused by the engine while it is tracked
 * - a light is removed when its projectile is gone, the feature is switched off, a level is loading or the engine deleted it
 */
namespace ssa::Game::AttackLights
{
    namespace
    {
        constexpr size_t kMaxLights = 32;
        constexpr size_t kMaxObservations = 128;
        constexpr size_t kMaxListNodes = 4096; // corrupted-list guard
        constexpr ULONGLONG kTestLightMs = 3000;
        constexpr ULONGLONG kRecordTimeoutMs = 500;

        using Data::AttackLightKey;
        using Data::AttackLightProfile;

        struct Appearance
        {
            float colour[3];
            float intensity;
            float radius;
        };

        // a projectile that could get a light
        struct Candidate
        {
            SharedPtr<EntityObject> owner;
            void* control; // control block of the projectile component
            int index;
            AttackLightKey key;
        };

        // a light the mod created
        struct Active
        {
            SharedPtr<Light> light; // the mod holds one strong reference
            void* projectileControl; // weak reference, null for the test light
            AttackLightKey key;
            const EntityObject* owner;
            int index;
            ULONGLONG expires; // 0 = never
        };

        std::vector<Active> s_active;
        std::vector<Observation> s_observations;
        TestLightSettings s_testSettings;
        std::string s_status;
        ULONGLONG s_recordUntil = 0;
        size_t s_matched = 0;
        uint32_t s_created = 0;
        bool s_initialized = false;
        bool s_testRequested = false;

        void EnsureInitialized()
        {
            if (s_initialized) return;
            s_initialized = true;

            s_active.reserve(kMaxLights);
            s_observations.reserve(kMaxObservations);
        }

        Appearance AppearanceOf(const AttackLightProfile& profile)
        {
            return {{profile.colour[0], profile.colour[1], profile.colour[2]}, profile.intensity, profile.radius};
        }

        Appearance TestAppearance()
        {
            return {{0.35f, 0.8f, 1.5f}, s_testSettings.intensity, s_testSettings.radius};
        }

        // SharedPtr / WeakPtr of an entity object: still referenced and not marked for deletion
        template <class Ref>
        bool IsLive(const Ref& ref)
        {
            return Shared::IsAlive(ref) && !ref.ptr->isDeleted();
        }

        bool HasLight(const SharedPtr<EntityObject>& owner)
        {
            return owner.ptr->findComponent(kCrcLight) != nullptr;
        }

        // the owner's light is one the mod created
        bool HasOurLight(const SharedPtr<EntityObject>& owner)
        {
            const void* component = owner.ptr->findComponent(kCrcLight);
            return component && std::any_of(s_active.begin(), s_active.end(),
                                            [component](const Active& active)
                                            {
                                                return active.light.ptr == component;
                                            });
        }

        void Observe(const Candidate& candidate, const Character& player)
        {
            auto it = std::find_if(s_observations.begin(), s_observations.end(),
                                   [&candidate](const Observation& observation)
                                   {
                                       return observation.key == candidate.key;
                                   });
            if (it == s_observations.end())
            {
                if (s_observations.size() >= kMaxObservations) return;
                const auto* desc = Game::instance()->getCharacterDesc(player.nameID);
                s_observations.push_back({candidate.key, desc ? desc->m_Name.c_str() : "Unknown character"});
                it = std::prev(s_observations.end());
            }
            ++it->live;
            it->nativeLight |= HasLight(candidate.owner) && !HasOurLight(candidate.owner);
        }

        // projectiles of the local players that are still flying
        std::vector<Candidate> FindProjectiles(const bool observe)
        {
            std::vector<Candidate> result;

            std::vector<Character*> players;
            for (const auto& ref : *Character::instanceSkylandersList())
                if (ref.mPtr && ref.mPtr->isPlayer())
                    players.push_back(ref.mPtr);
            if (players.empty()) return result;

            auto* world = World::instance();
            if (!world) return result;

            // the same component can be in more than one of these lists
            for (auto* list : {&world->componentList, &world->compInitList, &world->compPostInitList})
            {
                if (list->empty()) continue;

                size_t guard = 0;
                for (auto it = list->begin(); it.cur && it != list->end() && guard < kMaxListNodes; ++it, ++guard)
                {
                    const WeakPtr<Component> weak = *it;
                    if (!Shared::IsAlive(weak)) continue;

                    const Projectile* projectile = Projectile::fromComponent(weak.ptr);
                    if (!projectile || !projectile->isActive()) continue;

                    const WeakPtr<EntityObject> attacker = projectile->attacker;
                    if (!IsLive(attacker)) continue;

                    const auto player = std::find_if(players.begin(), players.end(),
                                                     [&attacker](const Character* p)
                                                     {
                                                         return p->m_pObject == attacker.ptr;
                                                     });
                    if (player == players.end()) continue;

                    const SharedPtr<EntityObject> owner = projectile->base.owner;
                    const int index = projectile->base.matrixIndex;
                    if (!IsLive(owner) || index < 0) continue;

                    if (std::any_of(result.begin(), result.end(),
                                    [&weak](const Candidate& c) { return c.control == weak.ctrl; }))
                        continue;

                    result.push_back({owner, weak.ctrl, index, {(*player)->nameID, projectile->triggerScriptName}});
                    if (observe) Observe(result.back(), **player);
                }
            }
            return result;
        }

        // creates a light on the owner object; null if the owner is gone or has a light already
        SharedPtr<Light> CreateLight(const SharedPtr<EntityObject>& owner, const int index, const Appearance& appearance)
        {
            if (!IsLive(owner) || index < 0 || HasLight(owner)) return {};

            // the factory copies the values into the light, but the definition has to have a fixed address while it reads them
            static LxLightDefinition definition;
            float colour[3];
            for (size_t i = 0; i < 3; ++i)
                colour[i] = appearance.colour[i] * appearance.intensity;
            definition.set(colour, appearance.radius);

            LxContext context{};
            context.owner = owner;
            context.matrixIndex = index;
            const LxBrowser browser = definition.browser();

            Light* light = Light::createFromLX(context, browser);
            if (!light) return {};

            // our own reference keeps the storage valid even after the engine deleted the light
            Shared::AddStrong(light->base.self.ctrl);
            return {light, light->base.self.ctrl};
        }

        bool Create(const SharedPtr<EntityObject>& owner, const int index, void* projectileControl, const AttackLightKey& key, const Appearance& appearance, const ULONGLONG expires = 0)
        {
            if (s_active.size() >= kMaxLights) return false;

            const SharedPtr<Light> light = CreateLight(owner, index, appearance);
            if (!light.ptr) return false;

            if (projectileControl) Shared::AddWeak(projectileControl);
            s_active.push_back({light, projectileControl, key, owner.ptr, index, expires});
            ++s_created;
            LogDebug("[AttackLights] Created %p for character %08x trigger tag %08x", light.ptr, key.character, key.triggerTag);
            return true;
        }

        void Apply(Light& light, const Appearance& appearance)
        {
            for (size_t i = 0; i < 3; ++i)
                light.colour[i] = appearance.colour[i] * appearance.intensity;
            light.range = appearance.radius;
        }

        void Remove(Active& active)
        {
            // owner teardown may already have unlinked the light, it must not be deleted twice
            if (!active.light.ptr->isDeleted())
                active.light.ptr->destroy();

            Shared::ReleaseStrong(active.light.ctrl);
            Shared::ReleaseWeak(active.projectileControl);
        }

        void UpdateActive(const std::vector<Candidate>& candidates, const bool enabled, const bool transitioning, const ULONGLONG now)
        {
            for (auto it = s_active.begin(); it != s_active.end();)
            {
                const bool projectileLight = it->projectileControl != nullptr;
                const AttackLightProfile* profile = projectileLight ? Data::FindAttackLightProfile(it->key) : nullptr;

                const bool projectileGone = projectileLight && std::none_of(candidates.begin(), candidates.end(),
                                                                            [&it](const Candidate& c)
                                                                            {
                                                                                return c.control == it->
                                                                                    projectileControl && c.key == it->
                                                                                    key &&
                                                                                    c.owner.ptr == it->owner && c.index
                                                                                    == it->index;
                                                                            });
                const bool disabled = projectileLight && (!enabled || !profile);
                const bool expired = it->expires && now >= it->expires;

                if (transitioning || disabled || projectileGone || expired || it->light.ptr->isDeleted())
                {
                    Remove(*it);
                    it = s_active.erase(it);
                }
                else
                {
                    Apply(*it->light.ptr, profile ? AppearanceOf(*profile) : TestAppearance());
                    ++it;
                }
            }
        }

        void UpdateTestLight(const bool transitioning, const ULONGLONG now)
        {
            if (!s_testRequested) return;
            s_testRequested = false;

            bool made = false;
            for (const auto& ref : *Character::instanceSkylandersList())
            {
                if (!ref.mPtr || !ref.mPtr->isPlayer1()) continue;

                made = !transitioning && Create(ref.mPtr->objectRef(), 0, nullptr, {}, TestAppearance(), now + kTestLightMs);
                break;
            }
            s_status = made ? "Test light created for three seconds." : "No eligible player owner, or owner already has a light.";
        }

        void CreateMissing(const std::vector<Candidate>& candidates)
        {
            for (const auto& candidate : candidates)
            {
                const AttackLightProfile* profile = Data::FindAttackLightProfile(candidate.key);
                if (!profile) continue;

                const bool tracked = std::any_of(s_active.begin(), s_active.end(),
                                                 [&candidate](const Active& active)
                                                 {
                                                     return active.projectileControl == candidate.control;
                                                 });
                if (!tracked)
                    Create(candidate.owner, candidate.index, candidate.control, candidate.key, AppearanceOf(*profile));
            }
        }
    } // namespace

    void Update()
    {
        EnsureInitialized();

        const ULONGLONG now = GetTickCount64();
        const bool enabled = g_config.attackLights;
        const bool observe = now < s_recordUntil;

        // nothing to do and nothing to clean up
        if (!enabled && !observe && !s_testRequested && s_active.empty())
        {
            s_matched = 0;
            return;
        }

        const bool transitioning = Game::instance()->isTransitioningLevel();

        if (observe)
            for (auto& observation : s_observations)
                observation.live = 0;

        const auto candidates = transitioning ? std::vector<Candidate>{} : FindProjectiles(observe);
        s_matched = static_cast<size_t>(std::count_if(candidates.begin(), candidates.end(),
                                                      [](const Candidate& c)
                                                      {
                                                          return Data::FindAttackLightProfile(c.key) != nullptr;
                                                      }));

        UpdateActive(candidates, enabled, transitioning, now);
        UpdateTestLight(transitioning, now);
        if (enabled)
            CreateMissing(candidates);
    }

    Stats GetStats() { return {s_matched, s_active.size(), kMaxLights, s_created}; }
    const std::string& GetStatus() { return s_status; }
    const std::vector<Observation>& GetObservations() { return s_observations; }
    TestLightSettings& GetTestLightSettings() { return s_testSettings; }

    void KeepRecording() { s_recordUntil = GetTickCount64() + kRecordTimeoutMs; }
    void ClearObservations() { s_observations.clear(); }
    void SpawnTestLight() { s_testRequested = true; }
} // namespace ssa::Game::AttackLights
