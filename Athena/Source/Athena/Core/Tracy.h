#pragma once

#include "Athena/Core/BuildConfiguration.h"

#if ATN_ENABLE_TRACY
	#ifndef TRACY_ENABLE
		#define TRACY_ENABLE 1
	#endif

	#include <tracy/Tracy.hpp>

	#define TRACY_FRAME_MARK() FrameMark
	#define TRACY_PROFILE_FUNC() ZoneScoped
	#define TRACY_PROFILE_SCOPE(name) ZoneScopedN(name)
#else
	#define TRACY_FRAME_MARK()
	#define TRACY_PROFILE_FUNC()
	#define TRACY_PROFILE_SCOPE(name)
#endif
