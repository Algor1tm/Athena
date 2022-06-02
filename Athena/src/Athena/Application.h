#pragma once

#include "Core.h"
#include "Window.h"
#include "LayerStack.h"
#include "Athena/Renderer/Shader.h"
#include "Events/ApplicationEvent.h"
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
		bool OnWindowClose(const WindowCloseEvent& event);

		std::unique_ptr<Window> m_Window;
		ImGuiLayer* m_ImGuiLayer;
		bool m_Running;
		LayerStack m_LayerStack;

		unsigned int m_VertexArray, m_VertexBuffer, m_IndexBuffer;
		std::unique_ptr<Shader> m_Shader;
	private:
		static Application* s_Instance;
	};

	// Defined by user
	Application* CreateApplication();
}

