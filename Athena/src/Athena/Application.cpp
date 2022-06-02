#include "atnpch.h"
#include "Application.h"
#include "Log.h"

#include "glad/glad.h"
#include "Input.h"
#include "Math/Utils.h"


namespace Athena
{
	Application* Application::s_Instance = nullptr;

	static GLenum ShaderDataTypeToOpenGLBaseType(ShaderDataType type)
	{
		switch (type)
		{
			case ShaderDataType::Float:  return GL_FLOAT;
			case ShaderDataType::Float2: return GL_FLOAT;
			case ShaderDataType::Float3: return GL_FLOAT;
			case ShaderDataType::Float4: return GL_FLOAT;
			case ShaderDataType::Mat3:	 return GL_FLOAT;
			case ShaderDataType::Mat4:   return GL_FLOAT;
			case ShaderDataType::Int:    return GL_INT;
			case ShaderDataType::Int2:   return GL_INT;
			case ShaderDataType::Int3:   return GL_INT;
			case ShaderDataType::Int4:   return GL_INT;
			case ShaderDataType::Bool:   return GL_BOOL;
		}

		ATN_CORE_ASSERT(false, "Unknown ShaderDataType!");
		return 0;
	}

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

		float vertices[18] = {
			-0.5, -0.5f, 0.0f, 1, 0, 0,
			0.5f, -0.5f, 0.0f, 0, 1, 0,
			0.0f, 0.5f, 0.0f, 0, 0, 1
		};

		m_VertexBuffer.reset(VertexBuffer::Create(vertices, (uint32_t)std::size(vertices)));

		unsigned int indices[3] = { 0, 1, 2 };
		m_IndexBuffer.reset(IndexBuffer::Create(indices, (uint32_t)std::size(indices)));

		BufferLayout layout = { { ShaderDataType::Float3, "a_Position"}, 
								{ ShaderDataType::Float3, "a_Color"} };

		uint32_t index = 0;
		for (const auto& elem : layout)
		{
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
			glEnableVertexAttribArray(index);
			glVertexAttribPointer(index, 
				elem.GetComponentCount(),
				ShaderDataTypeToOpenGLBaseType(elem.Type), 
				elem.Normalized ? GL_TRUE : GL_FALSE, 
				layout.GetStride(), 
				reinterpret_cast<void*>((uint64_t)elem.Offset));

			++index;
		}

		std::string vertexSrc = R"(
			#version 330 core

			layout (location = 0) in vec3 a_Position;
			layout (location = 1) in vec3 a_Color;
			
			out vec4 Color;
			
			void main()
			{
				Color = vec4(a_Color, 1);
				gl_Position = vec4(a_Position, 1);
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
			glDrawElements(GL_TRIANGLES, m_IndexBuffer->GetCount(), GL_UNSIGNED_INT, nullptr);

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
