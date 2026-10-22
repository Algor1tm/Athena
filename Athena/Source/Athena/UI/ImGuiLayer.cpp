#include "ImGuiLayer.h"

#include "Athena/Core/Application.h"
#include "Athena/Core/FileSystem.h"
#include "Athena/Renderer/EngineRenderAssets.h"

#include <ImGui/imgui.h>
#include "ImGui/imgui_internal.h"

#include <ImGui/backends/imgui_impl_glfw.h>
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
			font = io.Fonts->AddFontFromFileTTF(path.string().c_str(), size, nullptr, io.Fonts->GetGlyphRangesCyrillic());
			loaded = (bool)font;
		}
		
		if(!loaded)
			ATN_LOG_ERROR(Renderer, "Failed to load ImGUI UI font '{}'!", path);

		return font;
	}

	// Rewrite of ImGui::UpdatePlatformWindows() and ImGui::RenderPlatformWindowsDefault() for render thread
	static void UpdatePlatformWindows()
	{
		ImGuiContext& g = *GImGui;
		IM_ASSERT(g.FrameCountEnded == g.FrameCount && "Forgot to call Render() or EndFrame() before UpdatePlatformWindows()?");
		IM_ASSERT(g.FrameCountPlatformEnded < g.FrameCount);
		g.FrameCountPlatformEnded = g.FrameCount;
		if (!(g.ConfigFlagsCurrFrame & ImGuiConfigFlags_ViewportsEnable))
			return;

		// Create/resize/destroy platform windows to match each active viewport.
		// Skip the main viewport (index 0), which is always fully handled by the application!
		for (int i = 1; i < g.Viewports.Size; i++)
		{
			ImGuiViewportP* viewport = g.Viewports[i];

			// Destroy platform window if the viewport hasn't been submitted or if it is hosting a hidden window
			// (the implicit/fallback Debug##Default window will be registering its viewport then be disabled, causing a dummy DestroyPlatformWindow to be made each frame)
			bool destroy_platform_window = false;
			destroy_platform_window |= (viewport->LastFrameActive < g.FrameCount - 1);
			destroy_platform_window |= (viewport->Window && !(viewport->Window->Active && !viewport->Window->Hidden));
			if (destroy_platform_window)
			{
				if (viewport->DestroyFrameNumber != 0)
				{
					if (viewport->DestroyFrameNumber <= g.FrameCount)
					{
						g.PlatformIO.Platform_DestroyWindow(viewport);
					}
				}
				else if (viewport->PlatformWindowCreated)
				{
					ENQUEUE_RENDER_COMMAND([viewport]() 
						{
							TRACY_PROFILE_SCOPE("ImGui::DestroyViewport");
							ImGuiContext& g = *GImGui;

							if (g.PlatformIO.Renderer_DestroyWindow)
								g.PlatformIO.Renderer_DestroyWindow(viewport);
						});

					viewport->DestroyFrameNumber = g.FrameCount + 2;

					const ImGuiID IMGUI_VIEWPORT_DEFAULT_ID = 0x11111111;
					// Don't clear PlatformWindowCreated for the main viewport, as we initially set that up to true in Initialize()
					// The righter way may be to leave it to the backend to set this flag all-together, and made the flag public.
					if (viewport->ID != IMGUI_VIEWPORT_DEFAULT_ID)
						viewport->PlatformWindowCreated = false;
				}

				viewport->ClearRequestFlags();
				continue;
			}

			// New windows that appears directly in a new viewport won't always have a size on their first frame
			if (viewport->LastFrameActive < g.FrameCount || viewport->Size.x <= 0 || viewport->Size.y <= 0)
				continue;

			// Create window
			const bool is_new_platform_window = (viewport->PlatformWindowCreated == false);
			if (is_new_platform_window)
			{
				IMGUI_DEBUG_LOG_VIEWPORT("[viewport] Create Platform Window %08X '%s'\n", viewport->ID, viewport->Window ? viewport->Window->Name : "n/a");
				g.PlatformIO.Platform_CreateWindow(viewport);
				if (g.PlatformIO.Renderer_CreateWindow != NULL)
				{
					ENQUEUE_RENDER_COMMAND([viewport]() 
						{
							TRACY_PROFILE_SCOPE("ImGui::CreateViewport");
							ImGuiContext& g = *GImGui;
							g.PlatformIO.Renderer_CreateWindow(viewport);
						});
				}
				g.PlatformWindowsCreatedCount++;
				viewport->LastNameHash = 0;
				viewport->LastPlatformPos = viewport->LastPlatformSize = ImVec2(FLT_MAX, FLT_MAX); // By clearing those we'll enforce a call to Platform_SetWindowPos/Size below, before Platform_ShowWindow (FIXME: Is that necessary?)
				viewport->LastRendererSize = viewport->Size;                                       // We don't need to call Renderer_SetWindowSize() as it is expected Renderer_CreateWindow() already did it.
				viewport->PlatformWindowCreated = true;
			}

			// Apply Position and Size (from ImGui to Platform/Renderer backends)
			if ((viewport->LastPlatformPos.x != viewport->Pos.x || viewport->LastPlatformPos.y != viewport->Pos.y) && !viewport->PlatformRequestMove)
				g.PlatformIO.Platform_SetWindowPos(viewport, viewport->Pos);
			if ((viewport->LastPlatformSize.x != viewport->Size.x || viewport->LastPlatformSize.y != viewport->Size.y) && !viewport->PlatformRequestResize)
				g.PlatformIO.Platform_SetWindowSize(viewport, viewport->Size);
			if ((viewport->LastRendererSize.x != viewport->Size.x || viewport->LastRendererSize.y != viewport->Size.y) && g.PlatformIO.Renderer_SetWindowSize)
			{
				ENQUEUE_RENDER_COMMAND([viewport]()
					{
						TRACY_PROFILE_SCOPE("ImGui::SetViewportSize");
						ImGuiContext& g = *GImGui;
						g.PlatformIO.Renderer_SetWindowSize(viewport, viewport->Size);
					});
			}
			viewport->LastPlatformPos = viewport->Pos;
			viewport->LastPlatformSize = viewport->LastRendererSize = viewport->Size;

			ImGuiWindow* window_for_title = viewport->Window->DockNodeAsHost ? viewport->Window->DockNodeAsHost->VisibleWindow : viewport->Window;
			// Update title bar (if it changed)
			if (window_for_title)
			{
				const char* title_begin = window_for_title->Name;
				char* title_end = (char*)(intptr_t)ImGui::FindRenderedTextEnd(title_begin);
				const ImGuiID title_hash = ImHashStr(title_begin, title_end - title_begin);
				if (viewport->LastNameHash != title_hash)
				{
					char title_end_backup_c = *title_end;
					*title_end = 0; // Cut existing buffer short instead of doing an alloc/free, no small gain.
					g.PlatformIO.Platform_SetWindowTitle(viewport, title_begin);
					*title_end = title_end_backup_c;
					viewport->LastNameHash = title_hash;
				}
			}

			// Update alpha (if it changed)
			if (viewport->LastAlpha != viewport->Alpha && g.PlatformIO.Platform_SetWindowAlpha)
				g.PlatformIO.Platform_SetWindowAlpha(viewport, viewport->Alpha);
			viewport->LastAlpha = viewport->Alpha;

			// Optional, general purpose call to allow the backend to perform general book-keeping even if things haven't changed.
			if (g.PlatformIO.Platform_UpdateWindow)
				g.PlatformIO.Platform_UpdateWindow(viewport);

			if (is_new_platform_window)
			{
				// On startup ensure new platform window don't steal focus (give it a few frames, as nested contents may lead to viewport being created a few frames late)
				if (g.FrameCount < 3)
					viewport->Flags |= ImGuiViewportFlags_NoFocusOnAppearing;

				// Show window
				g.PlatformIO.Platform_ShowWindow(viewport);
			}

			// Clear request flags
			viewport->ClearRequestFlags();
		}


		// Skip the main viewport (index 0), which is always fully handled by the application!
		ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
		for (int i = 1; i < platform_io.Viewports.Size; i++)
		{
			ImGuiViewport* viewport = platform_io.Viewports[i];
			if (viewport->Flags & ImGuiViewportFlags_IsMinimized)
				continue;

			ENQUEUE_RENDER_COMMAND([viewport]() 
				{
					TRACY_PROFILE_SCOPE("ImGui::RenderViewport");
					ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
					if (platform_io.Platform_RenderWindow) platform_io.Platform_RenderWindow(viewport, nullptr);
					if (platform_io.Renderer_RenderWindow) platform_io.Renderer_RenderWindow(viewport, nullptr);
				});

		}
		for (int i = 1; i < platform_io.Viewports.Size; i++)
		{
			ImGuiViewport* viewport = platform_io.Viewports[i];
			if (viewport->Flags & ImGuiViewportFlags_IsMinimized)
				continue;

			ENQUEUE_RENDER_COMMAND([viewport]() 
				{
					TRACY_PROFILE_SCOPE("ImGui::ViewportSwapBuffers");
					ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
					if (platform_io.Platform_SwapBuffers) platform_io.Platform_SwapBuffers(viewport, nullptr);
					if (platform_io.Renderer_SwapBuffers) platform_io.Renderer_SwapBuffers(viewport, nullptr);
				});
		}
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
		return Ref<ImGuiLayer>::Create();
	}

	void ImGuiLayer::UpdateImGuiTheme()
	{
		ImGuiStyle& style = ImGui::GetStyle();
		auto& colors = style.Colors;

		style.FrameRounding = m_Theme.Style.FrameRounding;
		style.FrameBorderSize = m_Theme.Style.FrameBorderSize;
		style.ScrollbarSize = m_Theme.Style.ScrollbarSize;
		style.WindowRounding = m_Theme.Style.WindowRounding;
		style.FramePadding = { m_Theme.Style.FramePadding.x, m_Theme.Style.FramePadding.y };

		// Titlebar
		colors[ImGuiCol_TitleBg] = ImColor(m_Theme.Titlebar);
		colors[ImGuiCol_TitleBgActive] = ImColor(m_Theme.Titlebar);

		// Docking
		colors[ImGuiCol_DockingPreview] = ImColor(m_Theme.Accent);

		// Frame BG
		colors[ImGuiCol_FrameBg] = ImColor(m_Theme.FrameBg);
		colors[ImGuiCol_CheckboxSelectedBg] = ImColor(m_Theme.FrameBg);
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

	void* ImGuiLayer::GetTextureID(const Ref<TextureView>& texture)
	{
		return m_ImGuiBackend->GetTextureID(texture);
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
		Window& window = app.GetWindow();
		float scaleFactor = window.GetDPIScaleFactor();
		io.FontGlobalScale = scaleFactor;
		style.ScaleAllSizes(scaleFactor);

		const FilePath& resources = app.GetConfig().EngineResourcesPath;
		FilePath defaultFontPath = resources / "Fonts/Open_Sans/OpenSans-Medium.ttf";
		FilePath boldFontPath = resources / "Fonts/Open_Sans/OpenSans-Bold.ttf";

		io.FontDefault = TryLoadImGuiFont(defaultFontPath, 16.f);
		TryLoadImGuiFont(boldFontPath, 16.f);
		TryLoadImGuiFont(defaultFontPath, 22.f);

		m_ImGuiBackend = ImGuiBackend::Create();
		m_ImGuiBackend->Init_GT(window.GetNativeWindow());

		ENQUEUE_RENDER_COMMAND([this]()
			{
				m_ImGuiBackend->Init_RT();
			});


		ATN_LOG_INFO(Renderer, "Init ImGui(Viewports enable = {0}, Docking enable = {1})",
			bool(io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable), bool(io.ConfigFlags & ImGuiConfigFlags_DockingEnable));
	}

	void ImGuiLayer::OnDetach()
	{
		ENQUEUE_RENDER_COMMAND([imGuiBackend = m_ImGuiBackend]() mutable
		{
			imGuiBackend.Release();

			ATN_LOG_INFO(Renderer, "Destroyed ImGui context!");
		});
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
		Application& app = Application::Get();
		Window& window = app.GetWindow();

		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = ImVec2((float)event.GetWidth(), (float)event.GetHeight());

		return false;
	}

	void ImGuiLayer::Begin()
	{
		TRACY_PROFILE_FUNC();

		ENQUEUE_RENDER_COMMAND([imguiBackend = m_ImGuiBackend]()
			{
				imguiBackend->NewFrame_RT();
			});

		ImGui_ImplGlfw_NewFrame();

		m_ImGuiBackend->NewFrame_GT();
		ImGui::NewFrame();
		ImGuizmo::BeginFrame();
	}

	void ImGuiLayer::End(bool minimized)
	{
		TRACY_PROFILE_FUNC();
		ImGuiIO& io = ImGui::GetIO();

		ImGui::Render();
		m_ImGuiBackend->SwapDrawData();

		if (!minimized)
		{
			ENQUEUE_RENDER_COMMAND([imGuiBackend = m_ImGuiBackend]()
				{
					imGuiBackend->RenderDrawData();
				});
		}
		
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			UpdatePlatformWindows();
		}
	}

	void ImGuiLayer::OnSwapChainRecreate()
	{
		TRACY_PROFILE_FUNC();
		m_ImGuiBackend->OnSwapChainRecreate();
	}
}
