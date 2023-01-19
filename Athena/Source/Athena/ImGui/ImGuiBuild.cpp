#define IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#include <glad/glad.h>

#include <backends/imgui_impl_glfw.cpp>
#include <backends/imgui_impl_opengl3.cpp>

#ifdef ATN_PLATFORM_WINDOWS
	#include <backends/imgui_impl_win32.cpp>
	#include <backends/imgui_impl_dx11.cpp>
#endif
