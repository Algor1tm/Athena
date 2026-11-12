#pragma once

#include "Athena/Core/Core.h"

#include <comdef.h>


namespace Athena
{
    inline bool HandleHRESULT(HRESULT hr)
    {
        if (SUCCEEDED(hr))
            return true;

        _com_error comError(hr);
        std::wstring message(comError.ErrorMessage());

        ATN_LOG_ERROR(Debug, "HRESULT Error: {}", ToString(message));

        return false;
    }

#if ATN_ENABLE_CHECKS
    #define CHECK_HR(expr) checkf(::Athena::HandleHRESULT(expr))
#else
    #define CHECK_HR(expr) expr
#endif
}
