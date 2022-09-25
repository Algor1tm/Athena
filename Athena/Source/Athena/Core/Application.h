#pragma once

#include "Core.h"
#include "Window.h"
#include "LayerStack.h"
#include "Athena/Input/Events/ApplicationEvent.h"
#include "Athena/Core/Time.h"

#include "Athena/Renderer/OrthographicCamera.h"
#include "Athena/Renderer/RendererAPI.h"

#include "Athena/ImGui/ImGuiLayer.h"


namespace Athena
{
	struct ApplicationDescription
	{
		WindowDescription WindowDesc;

#ifdef ATN_PLATFORM_WINDOWS
		RendererAPI::API API = RendererAPI::Direct3D;
#else
		RendererAPI::API API = RendererAPI::OpenGL;
#endif
		bool UseImGui = true;
		bool UseConsole = true;
		Filepath WorkingDirectory = Filepath();
	};


	class ATHENA_API Application
	{
	public:
		Application(const ApplicationDescription& appdesc);
		virtual ~Application();

		void Run();
		void OnEvent(Event& event);

		void PushLayer(Ref<Layer> layer);
		void PushOverlay(Ref<Layer> layer);

		inline Ref<ImGuiLayer> GetImGuiLayer() { return m_ImGuiLayer; }
		inline Window& GetWindow() { return *m_Window; }

		void Close();

		inline static Application& Get() { return *s_Instance; }

	private:
		bool OnWindowClose(WindowCloseEvent& event);
		bool OnWindowResized(WindowResizeEvent& event);

		Scope<Window> m_Window;
		Ref<ImGuiLayer> m_ImGuiLayer;
		bool m_Running;
		bool m_Minimized;
		LayerStack m_LayerStack;

	private:
		static Application* s_Instance;
	};

	// Defined by user
	Application* CreateApplication();
}

