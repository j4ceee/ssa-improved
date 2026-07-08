#include "free_cam.h"

#include "addresses.h"
#include "config.h"
#include "game/camera.h"
#include "game/character.h"
#include "game/world.h"

/**
 * Strategy:
 * - patch vtable pointer inside Camera instance
 * - new vtable is copy of original with Update() function patched to use DebugCameraInstance::Update()
 * - all other vtable functions & camera fields stay the same
 */
namespace ssa::Game::FreeCam
{
    // -- Module state

    static void* s_vtable[32] = {}; // 32 entries just to be safe
    static void** s_origVtable = nullptr; // SpyroCam's original vtable pointer
    static bool s_active = false;
    static bool s_vtableReady = false;

    // -- Internal helpers

    static CameraViewInterface* GetPrimaryView()
    {
        auto* world = World::instance();
        if (!world) return nullptr;
        return world->cameraSet.primaryInterface();
    }

    static void EnsureVtableReady()
    {
        if (s_vtableReady) return;

        const auto* vi = GetPrimaryView();
        if (!vi || !vi->m_pCamera) return; // not ready yet; Toggle() will retry

        // read SpyroCam's current vtable pointer from the object header.
        s_origVtable = *reinterpret_cast<void***>(vi->m_pCamera);

        // copy the entire vtable
        for (int i = 0; i < 32; ++i)
            s_vtable[i] = s_origVtable[i];

        // replace only the Update slot (index 1 / byte offset +4)
        s_vtable[1] = GetAddress(DEBUG_CAM_UPDATE);

        s_vtableReady = true;
    }

    static void SetPlayerInputLock(const bool enable)
    {
        auto* list = Character::instanceSkylandersList();

        for (const auto& ref : *list)
        {
            auto* ch = ref.mPtr;
            if (!ch) continue;

            ch->setCinemaLock(enable);
        }
    }

    // -- Interaction

    void Toggle()
    {
        if (s_active)
        {
            s_active = false;

            SetPlayerInputLock(false);

            // restore original vtable pointer, it resumes normally next frame
            auto* vi = GetPrimaryView();
            if (vi && vi->m_pCamera)
                *reinterpret_cast<void***>(vi->m_pCamera) = s_origVtable;

            // clear custom vtable
            for (int i = 0; i < 32; ++i)
                s_vtable[i] = nullptr;
            s_origVtable = nullptr;
            s_vtableReady = false;

            return;
        }

        EnsureVtableReady();
        if (!s_vtableReady) return;

        auto* vi = GetPrimaryView();
        if (!vi || !vi->m_pCamera) return;

        // patch vtable pointer in original cam
        *reinterpret_cast<void***>(vi->m_pCamera) = s_vtable;
        SetPlayerInputLock(true);
        s_active = true;
    }

    void Update()
    {
        if (!s_active) return;
        SetPlayerInputLock(true);

        auto* vi = GetPrimaryView();
        if (vi)
        {
            vi->fov = static_cast<float>(g_config.freeCamFov);
            vi->m_pCamera->m_FOV = static_cast<float>(g_config.freeCamFov);
            vi->farPlane = 700.0f;
        }
    }

    bool IsActive() { return s_active; }
} // namespace ssa::Game::FreeCam
