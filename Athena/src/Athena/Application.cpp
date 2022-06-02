#include "atnpch.h"
#include "Application.h"
#include "Log.h"

#include "glad/glad.h"
#include "Input.h"
#include "Math/Utils.h"


namespace Athena
{
	Application* Application::s_Instance = nullptr;

	Application::Application()
		: m_Running(true)
	{
		ATN_CORE_ASSERT(s_Instance == nullptr, "Application already exists!");
		s_Instance = this;

		WindowDesc wdesc;
		wdesc.Width = 800;
		wdesc.Height = 600;

		m_Window = std::unique_ptr<Window>(Window::Create(wdesc));
		m_Window->SetEventCallback(BIND_MEMBER_EVENT_FN(Application::OnEvent));

		m_ImGuiLayer = new ImGuiLayer();
		PushOverlay(m_ImGuiLayer);

		glGenVertexArrays(1, &m_VertexArray);
		glBindVertexArray(m_VertexArray);

		glGenBuffers(1, &m_VertexBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, m_VertexBuffer);

		float vertices[18] = {
			-0.5, -0.5f, 0.0f, 1, 0, 0,
			0.5f, -0.5f, 0.0f, 0, 1, 0,
			0.0f, 0.5f, 0.0f, 0, 0, 1
		};

		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
		glEnableVertexAttribArray(1);
		unsigned int offset = 3 * sizeof(float);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (const void*)offset);

		glGenBuffers(1, &m_IndexBuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IndexBuffer);

		unsigned int indices[3] = { 0, 1, 2 };
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

		std::string vertexSrc = R"(
			#version 330 core

			layout (location = 0) in vec3 m_Position;
			layout (location = 1) in vec3 m_Color;
			
			out vec4 Color;
			
			void main()
			{
				Color = vec4(m_Color, 1);
				gl_Position = vec4(m_Position, 1);
			}
		)";

		std::string fragmentSrc = R"(
			#version 330 core
			
			in vec4 Color;
			layout(location = 0) out vec4 out_Color;

			void main()
			{
				out_Color = Color;
			}
		)";

		m_Shader = std::make_unique<Shader>(vertexSrc, fragmentSrc);
	}


	Application::~Application()
	{

	}


	void Application::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<WindowCloseEvent>(BIND_MEMBER_EVENT_FN(Application::OnWindowClose));

		for (LayerStack::iterator it = m_LayerStack.end(); it != m_LayerStack.begin();)
		{
			(*--it)->OnEvent(event);
			if (event.Handled)
				break;
		}
	}


	void Application::Run()
	{
		while (m_Running)
		{
			glClearColor(0.1f, 0.1f, 0.1f, 1);
			glClear(GL_COLOR_BUFFER_BIT);

			m_Shader->Bind();
			glBindVertexArray(m_VertexArray);
			glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, nullptr);

			for (Layer* layer: m_LayerStack)
				layer->OnUpdate();

			m_ImGuiLayer->Begin();
			for (Layer* layer: m_LayerStack)
				layer->OnImGuiRender();
			m_ImGuiLayer->End();

			m_Window->OnUpdate();
		}
	}


	void Application::PushLayer(Layer* layer)
	{
		m_LayerStack.PushLayer(layer);
	}


	void Application::PushOverlay(Layer* layer)
	{
		m_LayerStack.PushOverlay(layer);
	}


	bool Application::OnWindowClose(const WindowCloseEvent& event)
	{
		m_Running = false;
		return true;
	}
}
