#include "ImGuiLayer.h"

#include "Athena/Core/Application.h"

#include "Athena/Platform/OpenGL/GLImGuiLayerImpl.h"

#include "Athena/Renderer/Renderer.h"

#include <ImGui/imgui.h>
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
	
	Ref<ImGuiLayer> ImGuiLayer::Create()
	{
		auto imguiLayer = CreateRef<ImGuiLayer>();

		switch (Renderer::GetAPI())
		{
		case Renderer::API::OpenGL:
			imguiLayer->m_ImGuiImpl = CreateScope<GLImGuiLayerImpl>(); break;
		case Renderer::API::None:
			ATN_CORE_ASSERT(false, "Renderer API None is not supported"); break;
		default:
			ATN_CORE_ASSERT(false, "Unknown RendererAPI!"); break;
		}

		return imguiLayer;
	}

	void ImGuiLayer::SetDarkTheme()
	{
		ImGuiStyle& style = ImGui::GetStyle();
		auto& colors = style.Colors;

		style.FrameRounding = 3.f;
		style.FrameBorderSize = 1.f;


		// Widgets Active Items
		colors[ImGuiCol_SliderGrab] = ImVec4{ 0.f / 255.f, 112.f / 255.f, 224.f / 255.f, 1.0f };
		colors[ImGuiCol_SliderGrabActive] = ImVec4{ 0.f / 255.f, 112.f / 255.f, 224.f / 255.f, 1.0f };
		colors[ImGuiCol_ResizeGripHovered] = ImVec4{ 0.f / 255.f, 112.f / 255.f, 224.f / 255.f, 1.0f };
		colors[ImGuiCol_ResizeGripActive] = ImVec4{ 0.f / 255.f, 112.f / 255.f, 224.f / 255.f, 1.0f };
		colors[ImGuiCol_CheckMark] = ImVec4{ 0.f / 255.f, 112.f / 255.f, 224.f / 255.f, 1.0f };

		// Docking
		colors[ImGuiCol_DockingPreview] = ImVec4{ 0.f / 255.f, 112.f / 255.f, 224.f / 255.f, 1.0f };
		colors[ImGuiCol_DockingEmptyBg] = ImVec4{ 0.f / 255.f, 112.f / 255.f, 224.f / 255.f, 1.0f };

		// Frame BG
		colors[ImGuiCol_FrameBg] = ImVec4{ 14.f / 255.f, 14.f / 255.f, 14.f / 255.f, 1.0f };
		colors[ImGuiCol_FrameBgHovered] = ImVec4{ 51.f / 255.f, 51.f / 255.f, 51.f / 255.f, 1.0f };
		colors[ImGuiCol_FrameBgActive] = ImVec4{ 51.f / 255.f, 51.f / 255.f, 51.f / 255.f, 1.0f };

		// Headers
		colors[ImGuiCol_Header] = ImVec4{ 40.f / 255.f, 40.f / 255.f, 40.f / 255.f, 1.0f };
		colors[ImGuiCol_HeaderHovered] = ImVec4{ 53.f / 255.f, 51.f / 255.f, 51.f / 255.f, 1.0f };
		colors[ImGuiCol_HeaderActive] = ImVec4{ 53.f / 255.f, 51.f / 255.f, 51.f / 255.f, 1.0f };

		// Buttons
		colors[ImGuiCol_Button] = ImVec4{ 41.f / 255.f, 41.f / 255.f, 41.f / 255.f, 1.0f };
		colors[ImGuiCol_ButtonHovered] = ImVec4{ 53.f / 255.f, 51.f / 255.f, 51.f / 255.f, 1.0f };
		colors[ImGuiCol_ButtonActive] = ImVec4{ 53.f / 255.f, 51.f / 255.f, 51.f / 255.f, 1.0f };
		
		// Window
		colors[ImGuiCol_WindowBg] = ImVec4{ 30.f / 255.f, 30.f / 255.f, 30.f / 255.f, 1.0f };

		// Tabs
		colors[ImGuiCol_Tab] = ImVec4{ 30.f / 255.f, 30.f / 255.f, 30.f / 255.f, 1.0f };
		colors[ImGuiCol_TabHovered] = ImVec4{ 0.f / 255.f, 112.f / 255.f, 224.f / 255.f, 0.2f };
		colors[ImGuiCol_TabActive] = ImVec4{ 0.f / 255.f, 112.f / 255.f, 224.f / 255.f, 0.2f };
		colors[ImGuiCol_TabUnfocused] = ImVec4{ 30.f / 255.f, 30.f / 255.f, 30.f / 255.f, 1.0f };
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 30.f / 255.f, 30.f / 255.f, 30.f / 255.f, 1.0f };

		// Title
		colors[ImGuiCol_TitleBg] = ImVec4{ 17.f / 255.f, 17.f / 255.f, 17.f / 255.f, 1.0f };
		colors[ImGuiCol_TitleBgActive] = ImVec4{ 17.f / 255.f, 17.f / 255.f, 17.f / 255.f, 1.0f };
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 17.f / 255.f, 17.f / 255.f, 17.f / 255.f, 1.0f };

		// Lines
		colors[ImGuiCol_Separator] = ImVec4{ 15.f / 255.f, 15.f / 255.f, 15.f / 255.f, 1.0f };
		colors[ImGuiCol_TableBorderStrong] = ImVec4{ 15.f / 255.f, 15.f / 255.f, 15.f / 255.f, 1.0f };;
		colors[ImGuiCol_TableBorderLight] = ImVec4{ 15.f / 255.f, 15.f / 255.f, 15.f / 255.f, 1.0f };;
		colors[ImGuiCol_Border] = ImVec4{ 15.f / 255.f, 15.f / 255.f, 15.f / 255.f, 1.0f };
	}

	void ImGuiLayer::OnAttach()
	{
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

		m_ImGuiImpl->Init(app.GetWindow().GetNativeWindow());

		ATN_CORE_INFO("Init ImGui(Viewports enable = {0}, Docking enable = {1})", 
			bool(io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable), bool(io.ConfigFlags & ImGuiConfigFlags_DockingEnable));
		ATN_CORE_INFO("");
	}

	void ImGuiLayer::OnDetach()
	{
		m_ImGuiImpl->Shutdown();
		ImGui::DestroyContext();

		ATN_CORE_INFO("");
		ATN_CORE_INFO("Shutdown ImGui");
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
		m_ImGuiImpl->NewFrame();
		ImGui::NewFrame();
		ImGuizmo::BeginFrame();
	}

	void ImGuiLayer::End()
	{
		ImGuiIO& io = ImGui::GetIO();
		Application& app = Application::Get();
		io.DisplaySize = ImVec2((float)app.GetWindow().GetWidth(), (float)app.GetWindow().GetHeight());

		// Rendering
		ImGui::Render();
		m_ImGuiImpl->RenderDrawData();

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			m_ImGuiImpl->UpdateViewports();
		}
	}
}
