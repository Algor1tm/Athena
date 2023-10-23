#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Time.h"
#include "Athena/Core/LayerStack.h"
#include "Athena/Core/Window.h"

#include "Athena/Input/WindowEvent.h"

#include "Athena/Renderer/Renderer.h"

#include "Athena/Scripting/ScriptEngine.h"

#include <queue>


namespace Athena
{
	class ATHENA_API ImGuiLayer;

	struct AppConfig
	{
		String Name = "";
		bool EnableImGui = true;
		bool EnableConsole = true;
		FilePath WorkingDirectory = FilePath();
		FilePath EngineResourcesPath = FilePath();
	};

	struct ApplicationCreateInfo
	{
		AppConfig AppConfig;
		RendererConfig RendererConfig;
		ScriptConfig ScriptConfig;
		WindowCreateInfo WindowInfo;
	};


	class ATHENA_API Application
	{
	public:
		struct Statistics;

	public:
		Application(const ApplicationCreateInfo& appinfo);
		virtual ~Application();

		void Run();

		void PushLayer(Ref<Layer> layer);
		void PushOverlay(Ref<Layer> layer);

		const AppConfig GetConfig() const { return m_Config; }

		Ref<ImGuiLayer> GetImGuiLayer() { return m_ImGuiLayer; }
		Window& GetWindow() { return *m_Window; }
		const Statistics& GetStatistics() const { return m_Statistics; }

		void Close();

		inline static Application& Get() { return *s_Instance; }

	public:
		struct Statistics
		{
			Time FrameTime;
			Time Application_OnUpdate;
			Time Application_OnEvent;
			Time Application_OnImGuiRender;
			Time Window_SwapBuffers;
		};

	private:
		void ProcessEvents();
		void RenderImGui();

		void QueueEvent(const Ref<Event>& event);
		void OnEvent(Event& event);
		bool OnWindowClose(WindowCloseEvent& event);
		bool OnWindowResized(WindowResizeEvent& event);

		void CreateMainWindow(WindowCreateInfo info);
		void InitImGui();

	private:
		AppConfig m_Config;
		Scope<Window> m_Window;
		Ref<ImGuiLayer> m_ImGuiLayer;
		bool m_Running;
		bool m_Minimized;
		LayerStack m_LayerStack;

		std::queue<Ref<Event>> m_EventQueue;

		Statistics m_Statistics;

	private:
		static Application* s_Instance;
	};

	// Defined by user
	Application* CreateApplication();
}
