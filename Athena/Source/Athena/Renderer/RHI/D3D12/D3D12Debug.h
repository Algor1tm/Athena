#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Platform/Windows/WindowsDebug.h"
#include "Athena/Renderer/RHI/D3D12/D3D12Configuration.h"

#include <d3d12.h>


namespace Athena::D3D12
{
#define D3D12_CHECK(expr) CHECK_HR(expr)

    inline constexpr const char* D3D12MessageCategoryToString(D3D12_MESSAGE_CATEGORY category)
    {
        switch (category)
        {
        case D3D12_MESSAGE_CATEGORY_APPLICATION_DEFINED:    return "APPLICATION_DEFINED";
        case D3D12_MESSAGE_CATEGORY_MISCELLANEOUS:          return "MISCELLANEOUS";
        case D3D12_MESSAGE_CATEGORY_INITIALIZATION:         return "INITIALIZATION";
        case D3D12_MESSAGE_CATEGORY_CLEANUP:                return "CLEANUP";
        case D3D12_MESSAGE_CATEGORY_COMPILATION:            return "COMPILATION";
        case D3D12_MESSAGE_CATEGORY_STATE_CREATION:         return "STATE_CREATION";
        case D3D12_MESSAGE_CATEGORY_STATE_SETTING:          return "STATE_SETTING";
        case D3D12_MESSAGE_CATEGORY_STATE_GETTING:          return "STATE_GETTING";
        case D3D12_MESSAGE_CATEGORY_RESOURCE_MANIPULATION:  return "RESOURCE_MANIPULATION";
        case D3D12_MESSAGE_CATEGORY_EXECUTION:              return "EXECUTION";
        case D3D12_MESSAGE_CATEGORY_SHADER:                 return "SHADER";
        default:                                            return "UNKNOWN_CATEGORY";
        }
    }

    inline void DebugCallback(D3D12_MESSAGE_CATEGORY Category, D3D12_MESSAGE_SEVERITY Severity, D3D12_MESSAGE_ID ID, LPCSTR pDescription, void* pContext)
    {
        String message = "D3D12 debug message: \n\t";
        message += fmt::format("Category: {}\n\t", D3D12MessageCategoryToString(Category));
        message += fmt::format("ID: {}\n\t", (int)ID);
        message += fmt::format("Message: {}", pDescription);

        switch (Severity)
        {
        case D3D12_MESSAGE_SEVERITY_MESSAGE:
            ATN_LOG_TRACE(D3D12, message);
            break;

        case D3D12_MESSAGE_SEVERITY_INFO:
            ATN_LOG_INFO(D3D12, message);
            break;

        case D3D12_MESSAGE_SEVERITY_WARNING:
            ATN_LOG_WARN(D3D12, message);
            break;

        case D3D12_MESSAGE_SEVERITY_ERROR:
            ATN_LOG_ERROR(D3D12, message);
            break;

        case D3D12_MESSAGE_SEVERITY_CORRUPTION:
            ATN_LOG_FATAL(D3D12, message);
            break;
        }

        ensuref(false);
    }

    inline std::wstring ToWString(const String& str)
    {
        if (str.empty()) 
            return L"";

        int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);

        std::wstring wstrTo(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);

        return wstrTo;
    }

#if ATN_ENABLE_GPU_DEBUG
    #define D3D12_SET_DEBUG_NAME(d3dObject, name) d3dObject->SetName(D3D12::ToWString(name).c_str())
#else
    #define D3D12_SET_DEBUG_NAME(d3dObject, name)
#endif
}
