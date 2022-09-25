#include "atnpch.h"

#include "ImGuiLayer.h"
#include "Athena/Core/Application.h"

#include "Athena/Renderer/Renderer.h"
#include "Athena/Platform/OpenGL/GLImGuiLayerImpl.h"
#include "Athena/Platform/Direct3D/D3D11ImGuiLayerImpl.h"

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
		case RendererAPI::API::OpenGL:
			imguiLayer->m_ImGuiImpl = CreateScope<GLImGuiLayerImpl>(); break;
		case RendererAPI::API::Direct3D:
			imguiLayer->m_ImGuiImpl = CreateScope<D3D11ImGuiLayerImpl>(); break;
		case RendererAPI::API::None:
			ATN_CORE_ASSERT(false, "Renderer API None is not supported"); break;
		default:
			ATN_CORE_ASSERT(false, "Unknown RendererAPI!"); break;
		}

		return imguiLayer;
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

		io.Fonts->AddFontFromFileTTF("Resources/Fonts/Open_Sans/OpenSans-Bold.ttf", 16.f);
		io.FontDefault = io.Fonts->AddFontFromFileTTF("Resources/Fonts/Open_Sans/OpenSans-Medium.ttf", 16.f);

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
