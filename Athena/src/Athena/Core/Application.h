#pragma once

#include "Core.h"
#include "Window.h"
#include "LayerStack.h"
#include "Athena/Events/ApplicationEvent.h"
#include "Athena/Core/Time.h"

#include "Athena/Renderer/Shader.h"
#include "Athena/Renderer/Buffer.h"
#include "Athena/Renderer/VertexArray.h"
#include "Athena/Renderer/OrthographicCamera.h"

#include "Athena/ImGui/ImGuiLayer.h"


namespace Athena
{
	class ATHENA_API Application
	{
	public:
		Application();
		Application(Application&) = delete;
		Application(Application&&) = delete;
		virtual ~Application();

		void Run();
		void OnEvent(Event& event);

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);

		inline Window& GetWindow() { return *m_Window; }

		inline static Application& Get() { return *s_Instance; }
	private:
		bool OnWindowClose(WindowCloseEvent& event);
		bool OnWindowResized(WindowResizedEvent& event);

		std::unique_ptr<Window> m_Window;
		ImGuiLayer* m_ImGuiLayer;
		bool m_Running;
		bool m_Minimized;
		LayerStack m_LayerStack;
	private:
		static Application* s_Instance;
	};

	// Defined by user
	Application* CreateApplication();
}

