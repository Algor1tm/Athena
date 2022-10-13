#ifdef FORCE_GLFW
	#include "Athena/Platform/GLFW/GLFWWindow.h"
#else
	#ifdef ATN_PLATFORM_WINDOWS
		#include "Athena/Platform/Windows/WindowsWindow.h"
	#else
		#include "Athena/Platform/GLFW/GLFWWindow.h"
	#endif
#endif
