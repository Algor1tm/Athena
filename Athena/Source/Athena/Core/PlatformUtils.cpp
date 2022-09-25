#include "atnpch.h"


#ifdef FORCE_GLFW
	#include "Athena/Platform/GLFW/GLFWPlatformUtils.h"
#else
	#ifdef ATN_PLATFORM_WINDOWS
		#include "Athena/Platform/Windows/WindowsPlatformUtils.h"
	#else
		#include "Athena/Platform/GLFW/GLFWPlatformUtils.h"
	#endif
#endif
