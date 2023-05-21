#include "GLImGuiLayerImpl.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#ifdef _MSC_VER
#pragma warning(push, 0)
#endif

#define IMGUI_API_IMPL
#include <ImGui/backends/imgui_impl_glfw.h>
#include <ImGui/backends/imgui_impl_opengl3.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif


namespace Athena
{
	void GLImGuiLayerImpl::Init(void* windowHandle)
	{
		ImGui_ImplGlfw_InitForOpenGL(reinterpret_cast<GLFWwindow*>(windowHandle), true);
		ImGui_ImplOpenGL3_Init("#version 460");
	}

	void GLImGuiLayerImpl::Shutdown()
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
	}

	void GLImGuiLayerImpl::NewFrame()
	{
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
	}

	void GLImGuiLayerImpl::RenderDrawData()
	{
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}

	void GLImGuiLayerImpl::UpdateViewports()
	{
		GLFWwindow* backup_current_context = glfwGetCurrentContext();
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		glfwMakeContextCurrent(backup_current_context);
	}
}
