#include <Athena.h>

#include "Athena/Platform/OpenGL/OpenGLShader.h"

#include "ImGui/imgui.h"


class ExampleLayer : public Athena::Layer
{
public:
	ExampleLayer()
		: Layer("Example"), m_Camera(-1.6f, 1.6f, -0.9f, 0.9f), 
		  m_CameraPosition(0.f), m_GridScale(0.1f)
	{
		float vertices[] = {
			-0.5, -0.5f, 0.0f,    1.f, 0.f, 0.f,    0.0f, 0.0f,
			0.5f, -0.5f, 0.0f,    0.5f, 1.f, 0,      1.0f, 0.0f,   
			0.5f, 0.5f, 0.0f,     0.f, 0.8f, 0.8f,   1.0f, 1.0f,
			-0.5f, 0.5f, 0.0f,    1.f, 0, 1.f,      0.0f, 1.0f
		};

		Athena::Ref<Athena::VertexBuffer> vertexBuffer;
		vertexBuffer.reset(Athena::VertexBuffer::Create(vertices, (uint32_t)std::size(vertices)));

		Athena::BufferLayout layout = { { Athena::ShaderDataType::Float3, "a_Position"},
										{ Athena::ShaderDataType::Float3, "a_Color"},
										{ Athena::ShaderDataType::Float2, "a_TexCoord"} };
		vertexBuffer->SetLayout(layout);


		Athena::Ref<Athena::IndexBuffer> indexBuffer;
		unsigned int indices[6] = { 0, 1, 2, 2, 3, 0 };
		indexBuffer.reset(Athena::IndexBuffer::Create(indices, (uint32_t)std::size(indices)));

		// Grid
		{
			m_GridVertexArray.reset(Athena::VertexArray::Create());
			m_GridVertexArray->AddVertexBuffer(vertexBuffer);
			m_GridVertexArray->SetIndexBuffer(indexBuffer);

			m_GridShader = Athena::Shader::Create("assets/shaders/Grid.glsl");
			m_Texture = Athena::Texture2D::Create("assets/textures/KomodoHype.png");

			std::dynamic_pointer_cast<Athena::OpenGLShader>(m_GridShader)->Bind();
			std::dynamic_pointer_cast<Athena::OpenGLShader>(m_GridShader)->UploadUniformInt("u_Texture", 0);
		}

		// Square
		{
			m_SquareVertexArray.reset(Athena::VertexArray::Create());
			m_SquareVertexArray->AddVertexBuffer(vertexBuffer);
			m_SquareVertexArray->SetIndexBuffer(indexBuffer);

			std::string vertexSrc = R"(
			#version 330 core

			layout (location = 0) in vec3 a_Position;
			layout (location = 1) in vec3 a_Color;
			
			out vec4 Color;			

			uniform mat4 u_ViewProjection;
			uniform mat4 u_Transform;
			
			void main()
			{
				Color = vec4(a_Color, 1);
				gl_Position = u_ViewProjection * u_Transform * vec4(a_Position, 1);
			}
		)";

			std::string fragmentSrc = R"(
			#version 330 core
			
			layout(location = 0) out vec4 out_Color;
			in vec4 Color;
			uniform vec3 u_Color;

			void main()
			{
				out_Color = mix(Color, vec4(u_Color, 1), 0.5f);
			}
		)";

			m_SquareShader = Athena::Shader::Create(vertexSrc, fragmentSrc);
		}
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
			m_GridScale += m_GridSpeed * seconds;
		else if (Athena::Input::IsKeyPressed(Athena::Key::C))
			m_GridScale -= m_GridSpeed * seconds;

		Athena::RenderCommand::Clear({ 0.1f, 0.1f, 0.1f, 1 });

		m_Camera.SetRotation(m_CameraRotation);
		m_Camera.SetPosition(m_CameraPosition);

		Athena::Renderer::BeginScene(m_Camera);
		
		Athena::Matrix4 scale = Athena::Scale(m_GridScale);

		for (int x = 0; x < 10; ++x)
		{
			for (int y = -5; y < 5; ++y)
			{
				Athena::Vector3 position = { x * 0.11f, y * 0.11f, 0.f };
				Athena::Matrix4 transform = scale * Athena::Translate(position);
				Athena::Renderer::Submit(m_GridShader, m_GridVertexArray, transform);
			}
		}

		std::dynamic_pointer_cast<Athena::OpenGLShader>(m_SquareShader)->Bind();
		std::dynamic_pointer_cast<Athena::OpenGLShader>(m_SquareShader)->UploadUniformFloat3("u_Color", m_SquareColor);

		m_Texture->Bind();
		Athena::Matrix4 transform = Athena::Scale({ 0.7f, 0.7f, 0.7f }) * Athena::Translate({ -0.5f, 0.f, 0.f });
		Athena::Renderer::Submit(m_SquareShader, m_SquareVertexArray, transform);

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
	Athena::Ref<Athena::Shader> m_GridShader;
	Athena::Ref<Athena::VertexArray> m_GridVertexArray;
	Athena::Ref<Athena::Texture2D> m_Texture;

	Athena::Ref<Athena::Shader> m_SquareShader;
	Athena::Ref<Athena::VertexArray> m_SquareVertexArray;

	Athena::OrthographicCamera m_Camera;

	Athena::Vector3 m_CameraPosition;
	float m_CameraSpeed = 2.f;

	float m_CameraRotation = 0;
	float m_CameraRotationSpeed = 50.f;

	Athena::Vector3 m_GridScale;
	float m_GridSpeed = 0.08f;
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
