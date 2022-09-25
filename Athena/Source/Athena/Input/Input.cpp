#include "atnpch.h"


#ifdef FORCE_GLFW
	#include "Athena/Platform/GLFW/GLFWInput.h"
#else
	#ifdef ATN_PLATFORM_WINDOWS
		#include "Athena/Platform/Windows/WindowsInput.h"
	#else
		#include "Athena/Platform/GLFW/GLFWInput.h"
	#endif
#endif
