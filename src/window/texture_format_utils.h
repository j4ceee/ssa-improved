#pragma once
#include <d3d9.h>
#include <algorithm>
#include <cstdint>

namespace ssa::TextureFormat
{
    // taken from ScreenGrab9.cpp
    inline size_t BitsPerPixel(_In_ D3DFORMAT fmt) noexcept
    {
        switch (static_cast<int>(fmt))
        {
        case D3DFMT_A32B32G32R32F:
            return 128;

        case D3DFMT_A16B16G16R16:
        case D3DFMT_Q16W16V16U16:
        case D3DFMT_A16B16G16R16F:
        case D3DFMT_G32R32F:
            return 64;

        case D3DFMT_A8R8G8B8:
        case D3DFMT_X8R8G8B8:
        case D3DFMT_A2B10G10R10:
        case D3DFMT_A8B8G8R8:
        case D3DFMT_X8B8G8R8:
        case D3DFMT_G16R16:
        case D3DFMT_A2R10G10B10:
        case D3DFMT_Q8W8V8U8:
        case D3DFMT_V16U16:
        case D3DFMT_X8L8V8U8:
        case D3DFMT_A2W10V10U10:
        case D3DFMT_D32:
        case D3DFMT_D24S8:
        case D3DFMT_D24X8:
        case D3DFMT_D24X4S4:
        case D3DFMT_D32F_LOCKABLE:
        case D3DFMT_D24FS8:
        case D3DFMT_INDEX32:
        case D3DFMT_G16R16F:
        case D3DFMT_R32F:
        #if !defined(D3D_DISABLE_9EX)
        case D3DFMT_D32_LOCKABLE:
        #endif
            return 32;

        case D3DFMT_R8G8B8:
            return 24;

        case D3DFMT_A4R4G4B4:
        case D3DFMT_X4R4G4B4:
        case D3DFMT_R5G6B5:
        case D3DFMT_L16:
        case D3DFMT_A8L8:
        case D3DFMT_X1R5G5B5:
        case D3DFMT_A1R5G5B5:
        case D3DFMT_A8R3G3B2:
        case D3DFMT_V8U8:
        case D3DFMT_CxV8U8:
        case D3DFMT_L6V5U5:
        case D3DFMT_G8R8_G8B8:
        case D3DFMT_R8G8_B8G8:
        case D3DFMT_D16_LOCKABLE:
        case D3DFMT_D15S1:
        case D3DFMT_D16:
        case D3DFMT_INDEX16:
        case D3DFMT_R16F:
        case D3DFMT_YUY2:
            // From DX docs, reference/d3d/enums/d3dformat.asp
            // (note how it says that D3DFMT_R8G8_B8G8 is "A 16-bit packed RGB format analogous to UYVY (U0Y0, V0Y1, U2Y2, and so on)")
        case D3DFMT_UYVY:
            return 16;

        case D3DFMT_R3G3B2:
        case D3DFMT_A8:
        case D3DFMT_A8P8:
        case D3DFMT_P8:
        case D3DFMT_L8:
        case D3DFMT_A4L4:
        case D3DFMT_DXT2:
        case D3DFMT_DXT3:
        case D3DFMT_DXT4:
        case D3DFMT_DXT5:
            // http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directshow/htm/directxvideoaccelerationdxvavideosubtypes.asp
        case MAKEFOURCC('A', 'I', '4', '4'):
        case MAKEFOURCC('I', 'A', '4', '4'):
        #if !defined(D3D_DISABLE_9EX)
        case D3DFMT_S8_LOCKABLE:
        #endif
            return 8;

        case D3DFMT_DXT1:
            return 4;

        case MAKEFOURCC('Y', 'V', '1', '2'):
            return 12;

        #if !defined(D3D_DISABLE_9EX)
        case D3DFMT_A1:
            return 1;
        #endif

        default:
            return 0;
        }
    }

    // taken from ScreenGrab9.cpp
    inline HRESULT GetSurfaceInfo(
        _In_ size_t width,
        _In_ size_t height,
        _In_ D3DFORMAT fmt,
        size_t* outNumBytes,
        _Out_opt_ size_t* outRowBytes,
        _Out_opt_ size_t* outNumRows) noexcept
    {
        uint64_t numBytes = 0;
        uint64_t rowBytes = 0;
        uint64_t numRows = 0;

        bool bc = false;
        bool packed = false;
        size_t bpe = 0;
        switch (static_cast<int>(fmt))
        {
        case D3DFMT_DXT1:
            bc = true;
            bpe = 8;
            break;

        case D3DFMT_DXT2:
        case D3DFMT_DXT3:
        case D3DFMT_DXT4:
        case D3DFMT_DXT5:
            bc = true;
            bpe = 16;
            break;

        case D3DFMT_R8G8_B8G8:
        case D3DFMT_G8R8_G8B8:
        case D3DFMT_UYVY:
        case D3DFMT_YUY2:
            packed = true;
            bpe = 4;
            break;

        default:
            break;
        }

        if (bc)
        {
            uint64_t numBlocksWide = 0;
            if (width > 0)
            {
                numBlocksWide = std::max<uint64_t>(1u, (uint64_t(width) + 3u) / 4u);
            }
            uint64_t numBlocksHigh = 0;
            if (height > 0)
            {
                numBlocksHigh = std::max<uint64_t>(1u, (uint64_t(height) + 3u) / 4u);
            }
            rowBytes = numBlocksWide * bpe;
            numRows = numBlocksHigh;
            numBytes = rowBytes * numBlocksHigh;
        }
        else if (packed)
        {
            rowBytes = ((uint64_t(width) + 1u) >> 1) * bpe;
            numRows = uint64_t(height);
            numBytes = rowBytes * height;
        }
        else
        {
            const size_t bpp = BitsPerPixel(fmt);
            if (!bpp)
                return E_INVALIDARG;

            rowBytes = (uint64_t(width) * bpp + 7u) / 8u; // round up to nearest byte
            numRows = uint64_t(height);
            numBytes = rowBytes * height;
        }

    #if defined(_M_IX86) || defined(_M_ARM) || defined(_M_HYBRID_X86_ARM64)
        static_assert(sizeof(size_t) == 4, "Not a 32-bit platform!");
        if (numBytes > UINT32_MAX || rowBytes > UINT32_MAX || numRows > UINT32_MAX)
            return HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);
    #else
        static_assert(sizeof(size_t) == 8, "Not a 64-bit platform!");
    #endif

        if (outNumBytes)
        {
            *outNumBytes = static_cast<size_t>(numBytes);
        }
        if (outRowBytes)
        {
            *outRowBytes = static_cast<size_t>(rowBytes);
        }
        if (outNumRows)
        {
            *outNumRows = static_cast<size_t>(numRows);
        }

        return S_OK;
    }
}