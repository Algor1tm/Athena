#include "atnpch.h"

#include "ImGuiLayer.h"
#include "Athena/Core/Application.h"

// TEMPORARY
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define IMGUI_API_IMPL
#include <ImGui/backends/imgui_impl_glfw.h>
#include <ImGui/backends/imgui_impl_opengl3.h>

#include <ImGuizmo/ImGuizmo.h>


namespace Athena
{
	ImGuiLayer::ImGuiLayer()
		: Layer("ImGuiLayer")
	{

	}

	ImGuiLayer::~ImGuiLayer()
	{

	}

	void ImGuiLayer::SetDarkTheme()
	{
		auto& colors = ImGui::GetStyle().Colors;

		colors[ImGuiCol_WindowBg] = ImVec4{ 21.f / 255.f, 19.f / 255.f, 19.f / 255.f, 1.0f };

		// Hovered Red - ImVec4{ 250.f / 255.f, 70.f / 255.f, 70.f / 255.f, 1.f }; 
		// Headers
		colors[ImGuiCol_Header] = ImVec4{ 38.f / 255.f, 36.f / 255.f, 36.f / 255.f, 1.0f };
		colors[ImGuiCol_HeaderHovered] = ImVec4{ 53.f / 255.f, 51.f / 255.f, 51.f / 255.f, 1.0f }; 
		colors[ImGuiCol_HeaderActive] = ImVec4{ 46.f / 255.f, 44.f / 255.f, 44.f / 255.f, 1.0f }; 

		// Buttons
		colors[ImGuiCol_Button] = ImVec4{ 38.f / 255.f, 36.f / 255.f, 36.f / 255.f, 1.0f };
		colors[ImGuiCol_ButtonHovered] = ImVec4{ 53.f / 255.f, 51.f / 255.f, 51.f / 255.f, 1.0f };
		colors[ImGuiCol_ButtonActive] = ImVec4{ 46.f / 255.f, 44.f / 255.f, 44.f / 255.f, 1.0f };

		// Frame BG
		colors[ImGuiCol_FrameBg] = ImVec4{ 14.f / 255.f, 11.f / 255.f, 11.f / 255.f, 1.0f };
		colors[ImGuiCol_FrameBgHovered] = ImVec4{ 53.f / 255.f, 51.f / 255.f, 51.f / 255.f, 1.0f };
		colors[ImGuiCol_FrameBgActive] = ImVec4{ 46.f / 255.f, 44.f / 255.f, 44.f / 255.f, 1.0f };
		
		// Tabs
		colors[ImGuiCol_Tab] = ImVec4{ 28.f / 255.f, 26.f / 255.f, 26.f / 255.f, 1.0f };
		colors[ImGuiCol_TabHovered] = ImVec4{ 53.f / 255.f, 51.f / 255.f, 51.f / 255.f, 1.0f };
		colors[ImGuiCol_TabActive] = ImVec4{ 55.f / 255.f, 53.f / 255.f, 53.f / 255.f, 1.0f };
		colors[ImGuiCol_TabUnfocused] = ImVec4{ 28.f / 255.f, 26.f / 255.f, 26.f / 255.f, 1.0f };
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 40.f / 255.f, 38.f / 255.f, 38.f / 255.f, 1.0f };

		// Title
		colors[ImGuiCol_TitleBg] = ImVec4{ 28.f / 255.f, 26.f / 255.f, 26.f / 255.f, 1.0f };
		colors[ImGuiCol_TitleBgActive] = ImVec4{ 28.f / 255.f, 26.f / 255.f, 26.f / 255.f, 1.0f };
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
	}

	void ImGuiLayer::OnAttach()
	{
		ATN_PROFILE_FUNCTION();

		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
		//io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoTaskBarIcons;
		//io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoMerge;

		io.Fonts->AddFontFromFileTTF("assets/fonts/Open_Sans/OpenSans-Bold.ttf", 16.f);
		io.FontDefault = io.Fonts->AddFontFromFileTTF("assets/fonts/Open_Sans/OpenSans-Medium.ttf", 16.f);

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();
		//ImGui::StyleColorsClassic();

		// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}
		
		SetDarkTheme();

		Application& app = Application::Get();
		GLFWwindow* window = static_cast<GLFWwindow*>(app.GetWindow().GetNativeWindow());

		// Setup Platform/Renderer bindings
		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init("#version 410");
	}

	void ImGuiLayer::OnDetach()
	{
		ATN_PROFILE_FUNCTION();

		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void ImGuiLayer::OnEvent(Event& event)
	{
		if (m_BlockEvents)
		{
			ImGuiIO& io = ImGui::GetIO();
			event.Handled |= event.IsInCategory(EventCategoryMouse) & io.WantCaptureMouse;
			event.Handled |= event.IsInCategory(EventCategoryKeyboard) & io.WantCaptureKeyboard;
		}
	}

	void ImGuiLayer::Begin()
	{
		ATN_PROFILE_FUNCTION();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGuizmo::BeginFrame();
	}

	void ImGuiLayer::End()
	{
		ATN_PROFILE_FUNCTION();

		ImGuiIO& io = ImGui::GetIO();
		Application& app = Application::Get();
		io.DisplaySize = ImVec2((float)app.GetWindow().GetWidth(), (float)app.GetWindow().GetHeight());

		// Rendering
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}
	}
}
