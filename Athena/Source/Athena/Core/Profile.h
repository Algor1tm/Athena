#pragma once

#include <optick.h>


#define ATN_PROFILE_FRAME(frameName) OPTICK_FRAME(frameName)
#define ATN_PROFILER_SHUTDOWN() OPTICK_SHUTDOWN()

#define ATN_PROFILE_FUNC() OPTICK_EVENT()
#define ATN_PROFILE_SCOPE(name) OPTICK_EVENT(name)
