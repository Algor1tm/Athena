#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/ConsoleManager.h"
#include "Athena/Renderer/RHI/D3D12/D3D12RHI.h"


namespace Athena::D3D12
{
    inline uint32 GetMaxFramesInFlight()
    {
        return ConsoleManager::Get().FindCVar("r.MaxFramesInFlight")->GetInt();
    }

    inline DXGI_FORMAT TranslateFormat(Format format)
    {
        switch (format)
        {
        case Format::R8:              return DXGI_FORMAT_R8_UNORM;
        case Format::RG8:             return DXGI_FORMAT_R8G8_UNORM;
        case Format::RGBA8:           return DXGI_FORMAT_R8G8B8A8_UNORM;
        case Format::RGBA8_SRGB:      return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        case Format::R10G10B10A2:     return DXGI_FORMAT_R10G10B10A2_UNORM;

        case Format::R32F:            return DXGI_FORMAT_R32_FLOAT;
        case Format::RG16F:           return DXGI_FORMAT_R16G16_FLOAT;
        case Format::RG32F:           return DXGI_FORMAT_R32G32_FLOAT;
        case Format::R11G11B10UF:     return DXGI_FORMAT_R11G11B10_FLOAT;
        case Format::RGB32F:          return DXGI_FORMAT_R32G32B32_FLOAT;
        case Format::RGBA16F:         return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case Format::RGBA32F:         return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case Format::RGBA32UInt:      return DXGI_FORMAT_R32G32B32A32_UINT;

        case Format::DEPTH16:         return DXGI_FORMAT_D16_UNORM;
        case Format::DEPTH24STENCIL8: return DXGI_FORMAT_D24_UNORM_S8_UINT;
        case Format::DEPTH32F:        return DXGI_FORMAT_D32_FLOAT;
        }

        ensuref(false);
        return (DXGI_FORMAT)0;
    }
}
