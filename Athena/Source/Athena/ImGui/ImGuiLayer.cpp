#include "ImGuiLayer.h"

#include "Athena/Core/Application.h"
#include "Athena/Core/FileSystem.h"

#include "Athena/Renderer/Renderer.h"

#include "Athena/Platform/Vulkan/VulkanImGuiLayerImpl.h"

#include <ImGui/imgui.h>
#include <ImGuizmo/ImGuizmo.h>


namespace Athena
{
	static ImFont* TryLoadImGuiFont(const FilePath& path, float size)
	{
		ImFont* font = nullptr;
		bool loaded = false;
		if (FileSystem::Exists(path))
		{
			ImGuiIO& io = ImGui::GetIO();
			font = io.Fonts->AddFontFromFileTTF(path.string().c_str(), size);
			loaded = (bool)font;
		}
		
		if(!loaded)
			ATN_CORE_ERROR_TAG("ImGuiLayer", "Failed to load UI font '{}'!", path);

		return font;
	}

	ImGuiLayer::ImGuiLayer()
		: Layer("ImGuiLayer")
	{
		
	}

	ImGuiLayer::~ImGuiLayer()
	{

	}
	
	Ref<ImGuiLayer> ImGuiLayer::Create()
	{
		auto imguiLayer = Ref<ImGuiLayer>::Create();

		switch (Renderer::GetAPI())
		{
		case Renderer::API::Vulkan: imguiLayer->m_ImGuiImpl = Scope<VulkanImGuiLayerImpl>::Create(); break;
		case Renderer::API::None: imguiLayer->m_ImGuiImpl = nullptr; break;
		}

		return imguiLayer;
	}

	void ImGuiLayer::UpdateImGuiTheme()
	{
		ImGuiStyle& style = ImGui::GetStyle();
		auto& colors = style.Colors;

		style.FrameRounding = m_Theme.Style.FrameRounding;
		style.FrameBorderSize = m_Theme.Style.FrameBorderSize;
		style.ScrollbarSize = m_Theme.Style.ScrollbarSize;
		style.WindowRounding = m_Theme.Style.WindowRounding;


		// Titlebar
		colors[ImGuiCol_TitleBg] = ImColor(m_Theme.Titlebar);
		colors[ImGuiCol_TitleBgActive] = ImColor(m_Theme.Titlebar);

		// Docking
		colors[ImGuiCol_DockingPreview] = ImColor(m_Theme.Accent);

		// Frame BG
		colors[ImGuiCol_FrameBg] = ImColor(m_Theme.FrameBg);
		colors[ImGuiCol_FrameBgHovered] = ImColor(m_Theme.FrameBgActive);
		colors[ImGuiCol_FrameBgActive] = ImColor(m_Theme.FrameBgActive);

		// Headers
		colors[ImGuiCol_Header] = ImColor(m_Theme.Header);
		colors[ImGuiCol_HeaderHovered] = ImColor(m_Theme.HeaderActive);
		colors[ImGuiCol_HeaderActive] = ImColor(m_Theme.HeaderActive);

		// Buttons
		colors[ImGuiCol_Button] = ImColor(m_Theme.Button);
		colors[ImGuiCol_ButtonHovered] = ImColor(m_Theme.ButtonActive);
		colors[ImGuiCol_ButtonActive] = ImColor(m_Theme.ButtonActive);

		// Slider
		colors[ImGuiCol_SliderGrab] = ImColor(m_Theme.Accent);
		colors[ImGuiCol_SliderGrabActive] = ImColor(m_Theme.Accent);

		// Checkbox
		colors[ImGuiCol_CheckMark] = ImColor(m_Theme.Accent);

		// Text
		colors[ImGuiCol_Text] = ImColor(m_Theme.Text);

		// Resize Grip
		colors[ImGuiCol_ResizeGripHovered] = ImColor(m_Theme.Accent);
		colors[ImGuiCol_ResizeGripActive] = ImColor(m_Theme.Accent);

		// Window
		colors[ImGuiCol_WindowBg] = ImColor(m_Theme.Background);
		colors[ImGuiCol_PopupBg] = ImColor(m_Theme.BackgroundPopup);

		// Tabs
		colors[ImGuiCol_Tab] = ImColor(m_Theme.Tab);
		colors[ImGuiCol_TabHovered] = ImColor(m_Theme.TabActive);
		colors[ImGuiCol_TabActive] = ImColor(m_Theme.TabActive);
		colors[ImGuiCol_TabUnfocused] = ImColor(m_Theme.Tab);
		colors[ImGuiCol_TabUnfocusedActive] = ImColor(m_Theme.Tab);

		// Title
		colors[ImGuiCol_TitleBg] = ImColor(m_Theme.BackgroundDark);
		colors[ImGuiCol_TitleBgActive] = ImColor(m_Theme.BackgroundDark);
		colors[ImGuiCol_TitleBgCollapsed] = ImColor(m_Theme.BackgroundDark);

		// Lines
		colors[ImGuiCol_Separator] = ImColor(m_Theme.BackgroundDark);
		colors[ImGuiCol_TableBorderStrong] = ImColor(m_Theme.BackgroundDark);
		colors[ImGuiCol_TableBorderLight] = ImColor(m_Theme.BackgroundDark);
		colors[ImGuiCol_Border] = ImColor(m_Theme.BackgroundDark);
	}

	void* ImGuiLayer::GetTextureID(const Ref<Texture2D>& texture)
	{
		return m_ImGuiImpl->GetTextureID(texture);
	}

	void* ImGuiLayer::GetTextureID(const Ref<Image>& image)
	{
		return m_ImGuiImpl->GetTextureID(image);
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

		ImGui::StyleColorsDark();

		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}
		
		m_Theme = UI::Theme::DefaultDark();
		UpdateImGuiTheme();

		Application& app = Application::Get();

		const FilePath& resources = app.GetConfig().EngineResourcesPath;
		FilePath defaultFontPath = resources / "Fonts/Open_Sans/OpenSans-Medium.ttf";
		FilePath boldFontPath = resources / "Fonts/Open_Sans/OpenSans-Bold.ttf";

		io.FontDefault = TryLoadImGuiFont(defaultFontPath, 16.f);
		TryLoadImGuiFont(boldFontPath, 16.f);
		TryLoadImGuiFont(defaultFontPath, 22.f);

		m_ImGuiImpl->Init(app.GetWindow().GetNativeWindow());

		ATN_CORE_INFO_TAG("ImGuiLayer", "Init ImGui(Viewports enable = {0}, Docking enable = {1})",
			bool(io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable), bool(io.ConfigFlags & ImGuiConfigFlags_DockingEnable));
	}

	void ImGuiLayer::OnDetach()
	{
		m_ImGuiImpl->Shutdown();

		Renderer::SubmitResourceFree([]()
		{
			ImGui::DestroyContext();
		});

		ATN_CORE_INFO_TAG("ImGuiLayer", "Shutdown ImGui");
	}

	void ImGuiLayer::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<WindowResizeEvent>(ATN_BIND_EVENT_FN(ImGuiLayer::OnWindowResize));

		if (m_BlockEvents)
		{
			ImGuiIO& io = ImGui::GetIO();
			event.Handled |= event.IsInCategory(EventCategoryMouse) & io.WantCaptureMouse;
			event.Handled |= event.IsInCategory(EventCategoryKeyboard) & io.WantCaptureKeyboard;
		}
	}

	bool ImGuiLayer::OnWindowResize(WindowResizeEvent& event)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = ImVec2((float)event.GetWidth(), (float)event.GetHeight());

		return false;
	}

	void ImGuiLayer::Begin()
	{
		ATN_PROFILE_FUNC()
		m_ImGuiImpl->NewFrame();
		ImGui::NewFrame();
		ImGuizmo::BeginFrame();
	}

	void ImGuiLayer::End(bool minimized)
	{
		ATN_PROFILE_SCOPE("ImGuiLayer::End")
		ImGuiIO& io = ImGui::GetIO();

		ImGui::Render();

		if (!minimized)
		{
			m_ImGuiImpl->RenderDrawData(io.DisplaySize.x, io.DisplaySize.y);
		}

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
	}

	void ImGuiLayer::OnSwapChainRecreate()
	{
		ATN_PROFILE_FUNC()
		m_ImGuiImpl->OnSwapChainRecreate();
	}
}
