#pragma once

#include "Athena/Core/BuildConfiguration.h"

#if ATN_ENABLE_TRACY_CPU
	#include <tracy/Tracy.hpp>

	#define TRACY_FRAME_MARK() FrameMark
	#define TRACY_PROFILE_FUNC() ZoneScoped
	#define TRACY_PROFILE_SCOPE(name) ZoneScopedN(name)
#else
#if ATN_ENABLE_TRACY_GPU
	#include <tracy/Tracy.hpp>
	#define TRACY_FRAME_MARK() FrameMark
#endif

	#define TRACY_PROFILE_FUNC()
	#define TRACY_PROFILE_SCOPE(name)
#endif
