#include <Athena.h>

#include "Athena/Platform/OpenGL/OpenGLShader.h"

#include "ImGui/imgui.h"


class ExampleLayer : public Athena::Layer
{
public:
	ExampleLayer()
		: Layer("Example"), m_Camera(-1.6f, 1.6f, -0.9f, 0.9f), 
		  m_CameraPosition(0.f), m_SquareScale(0.1f)
	{
		float vertices[24] = {
			-0.5, -0.5f, 0.0f, 1.f, 0.2f, 0.2f,
			0.5f, -0.5f, 0.0f, 0.4f, 0.7f, 0,
			0.5f, 0.5f, 0.0f,  0.2f, 0.6f, 0.6f,
			-0.5f, 0.5f, 0.0f,  0.8f, 0, 0.8f
		};

		Athena::Ref<Athena::VertexBuffer> vertexBuffer;
		vertexBuffer.reset(Athena::VertexBuffer::Create(vertices, (uint32_t)std::size(vertices)));

		Athena::BufferLayout layout = { { Athena::ShaderDataType::Float3, "a_Position"},
										{ Athena::ShaderDataType::Float3, "a_Color"} };
		vertexBuffer->SetLayout(layout);


		Athena::Ref<Athena::IndexBuffer> indexBuffer;
		unsigned int indices[6] = { 0, 1, 2, 2, 3, 0 };
		indexBuffer.reset(Athena::IndexBuffer::Create(indices, (uint32_t)std::size(indices)));

		m_VertexArray.reset(Athena::VertexArray::Create());
		m_VertexArray->AddVertexBuffer(vertexBuffer);
		m_VertexArray->SetIndexBuffer(indexBuffer);

		std::string vertexSrc = R"(
			#version 330 core

			layout (location = 0) in vec3 a_Position;
			layout (location = 1) in vec3 a_Color;
			
			uniform mat4 u_ViewProjection;
			uniform mat4 u_Transform;
			
			void main()
			{
				gl_Position = u_ViewProjection * u_Transform * vec4(a_Position, 1);
			}
		)";

		std::string fragmentSrc = R"(
			#version 330 core
			
			layout(location = 0) out vec4 out_Color;
			uniform vec3 u_Color;

			void main()
			{
				out_Color = vec4(u_Color, 1);
			}
		)";

		m_Shader.reset(Athena::Shader::Create(vertexSrc, fragmentSrc));
	}

	void OnUpdate(Athena::Time frameTime) override
	{
		float seconds = frameTime.AsSeconds();

		if (Athena::Input::IsKeyPressed(Athena::Key::A))
			m_CameraPosition.x -= m_CameraSpeed * seconds;
		else if (Athena::Input::IsKeyPressed(Athena::Key::D))
			m_CameraPosition.x += m_CameraSpeed * seconds;
		else if (Athena::Input::IsKeyPressed(Athena::Key::W))
			m_CameraPosition.y += m_CameraSpeed * seconds;
		else if (Athena::Input::IsKeyPressed(Athena::Key::S ))
			m_CameraPosition.y -= m_CameraSpeed * seconds;

		else if (Athena::Input::IsKeyPressed(Athena::Key::Q))
			m_CameraRotation += m_CameraRotationSpeed * seconds;
		else if (Athena::Input::IsKeyPressed(Athena::Key::E))
			m_CameraRotation -= m_CameraRotationSpeed * seconds;

		else if (Athena::Input::IsKeyPressed(Athena::Key::Z))
			m_SquareScale += m_ScaleSpeed * seconds;
		else if (Athena::Input::IsKeyPressed(Athena::Key::C))
			m_SquareScale -= m_ScaleSpeed * seconds;

		Athena::RenderCommand::Clear({ 0.1f, 0.1f, 0.1f, 1 });

		m_Camera.SetRotation(m_CameraRotation);
		m_Camera.SetPosition(m_CameraPosition);

		Athena::Renderer::BeginScene(m_Camera);
		
		Athena::Matrix4 scale = Athena::Scale(m_SquareScale);

		m_Shader->Bind();
		std::dynamic_pointer_cast<Athena::OpenGLShader>(m_Shader)->UploadUniformFloat3("u_Color", m_SquareColor);
		for (int x = -5; x < 5; ++x)
		{
			for (int y = -5; y < 5; ++y)
			{
				Athena::Vector3 position = { x * 0.11f, y * 0.11f, 0.f };
				Athena::Matrix4 transform = scale * Athena::Translate(position);
				Athena::Renderer::Submit(m_Shader, m_VertexArray, transform);
			}
		}

		Athena::Renderer::EndScene();
	}

	void OnImGuiRender() override
	{
		ImGui::Begin("Settings");

		ImGui::ColorEdit3("Square Color", m_SquareColor.Data());

		ImGui::End();
	}

	void OnEvent(Athena::Event& event) override
	{
		using namespace Athena;
		
	}

private:
	Athena::Ref<Athena::Shader> m_Shader;
	Athena::Ref<Athena::VertexArray> m_VertexArray;

	Athena::OrthographicCamera m_Camera;

	Athena::Vector3 m_CameraPosition;
	float m_CameraSpeed = 2.f;

	float m_CameraRotation = 0;
	float m_CameraRotationSpeed = 50.f;

	Athena::Vector3 m_SquareScale;
	float m_ScaleSpeed = 0.08f;

	Athena::Vector3 m_SquareColor = { 0.2f, 0.3f, 0.8f };
};


class SandBox: public Athena::Application
{
public:
	SandBox() 
	{
		PushLayer(new ExampleLayer());
	}
	
	~SandBox()
	{

	}
};


Athena::Application* Athena::CreateApplication()
{
	return new SandBox();
}
