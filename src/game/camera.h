#pragma once

#include <cstdint>
#include "data_types.h"

#pragma pack(push, 1)
namespace ssa::Game
{
    constexpr float kDefaultFOV = 53.66799f;

    struct CameraViewInterface;

    struct CameraView
    {
        char                _pad0[0x4]; // +0x000
        RotTransMat4x3FP    cameraMat;  // +0x004 camera world-space transform
        RotTransMat4x3FP    lookMat;    // +0x034
        RotTransMat4x3FP    adjustMat;  // +0x064
        float               fov;        // +0x094
        float               nearPlane;  // +0x098
        float               farPlane;   // +0x09C
        float               m_left;     // +0x0A0
        float               m_top;      // +0x0A4
        float               m_right;    // +0x0A8
        float               m_bottom;   // +0x0AC
        RotTransMat4x3FP    viewMat;    // +0x0B0 inverse of cameraMat
        RotTransMat4x3FP    invView;    // +0x0E0 inverse of viewMat
    };
    static_assert(sizeof(CameraView) == 0x110);
    static_assert(offsetof(CameraView, cameraMat) == 0x004);
    static_assert(offsetof(CameraView, lookMat) == 0x034);
    static_assert(offsetof(CameraView, adjustMat) == 0x064);
    static_assert(offsetof(CameraView, fov) == 0x094);
    static_assert(offsetof(CameraView, viewMat) == 0x0B0);
    static_assert(offsetof(CameraView, invView) == 0x0E0);


    struct CameraInstance
    {
        char                    _pad0[0x4]; // +0x00 vtable
        uint32_t                m_Priority; // +0x04
        CameraInstance*         m_pNext;    // +0x08
        CameraViewInterface*    m_pView;    // +0x0C
        uint32_t                m_Type;     // +0x10
        float                   m_FOV;      // +0x14
    };
    static_assert(sizeof(CameraInstance) == 0x18);
    static_assert(offsetof(CameraInstance, m_FOV) == 0x14);


    struct CameraViewInterface : CameraView
    {
        CameraInstance* m_pCamera;      // +0x110
        float           m_fCamFOVSpd;   // +0x114
        void*           _shakes[2];     // +0x118
    };
    static_assert(sizeof(CameraViewInterface) == 0x120);


    struct CameraSet
    {
        CameraView* views[4];       // +0x00 one per viewport; views[0] = primary
        int32_t     activeViews;    // +0x10 number of slots currently in use
        int32_t     maxViews;       // +0x14

        [[nodiscard]] CameraViewInterface* primaryInterface() const
        {
            return static_cast<CameraViewInterface*>(views[0]);
        }
    };
    static_assert(sizeof(CameraSet) == 0x18);
    static_assert(offsetof(CameraSet, views) == 0x00);
    static_assert(offsetof(CameraSet, activeViews) == 0x10);
    static_assert(offsetof(CameraSet, maxViews) == 0x14);
} // namespace ssa::Game
#pragma pack(pop)