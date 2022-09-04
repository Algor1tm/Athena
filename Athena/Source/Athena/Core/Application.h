#pragma once

#include "Core.h"
#include "Window.h"
#include "LayerStack.h"
#include "Athena/Input/Events/ApplicationEvent.h"
#include "Athena/Core/Time.h"

#include "Athena/Renderer/Shader.h"
#include "Athena/Renderer/Buffer.h"
#include "Athena/Renderer/VertexArray.h"
#include "Athena/Renderer/OrthographicCamera.h"

#include "Athena/ImGui/ImGuiLayer.h"


namespace Athena
{
	struct ApplicationDESC
	{
		uint32 WindowWidth = 1600;
		uint32 WindowHeight = 900;
		String Title = "Athena App";
		bool VSync;

		bool UseImGui = true;
		Filepath WorkingDirectory = Filepath();
	};


	class ATHENA_API Application
	{
	public:
		Application(const ApplicationDESC& appdesc);
		virtual ~Application();

		void Run();
		void OnEvent(Event& event);

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);

		inline ImGuiLayer* GetImGuiLayer() { return m_ImGuiLayer; }
		inline Window& GetWindow() { return *m_Window; }

		void Close();

		inline static Application& Get() { return *s_Instance; }

	private:
		bool OnWindowClose(WindowCloseEvent& event);
		bool OnWindowResized(WindowResizeEvent& event);

		Scope<Window> m_Window;
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

