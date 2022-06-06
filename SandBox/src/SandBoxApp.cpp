#include <Athena.h>


class ExampleLayer : public Athena::Layer
{
public:
	ExampleLayer()
		: Layer("Example"), m_Camera(-1.333f, 1.333f, -1, 1), m_CameraPosition(0.f)
	{
		float vertices[24] = {
			-0.5, -0.5f, 0.0f, 1.f, 0.2f, 0.2f,
			0.5f, -0.5f, 0.0f, 0.4f, 0.7f, 0,
			0.5f, 0.5f, 0.0f,  0.2f, 0.6f, 0.6f,
			-0.5f, 0.5f, 0.0f,  0.8f, 0, 0.8f
		};

		std::shared_ptr<Athena::VertexBuffer> vertexBuffer;
		vertexBuffer.reset(Athena::VertexBuffer::Create(vertices, (uint32_t)std::size(vertices)));

		Athena::BufferLayout layout = { { Athena::ShaderDataType::Float3, "a_Position"},
										{ Athena::ShaderDataType::Float3, "a_Color"} };
		vertexBuffer->SetLayout(layout);


		std::shared_ptr<Athena::IndexBuffer> indexBuffer;
		unsigned int indices[6] = { 0, 1, 2, 2, 3, 0 };
		indexBuffer.reset(Athena::IndexBuffer::Create(indices, (uint32_t)std::size(indices)));

		m_VertexArray.reset(Athena::VertexArray::Create());
		m_VertexArray->AddVertexBuffer(vertexBuffer);
		m_VertexArray->SetIndexBuffer(indexBuffer);

		std::string vertexSrc = R"(
			#version 330 core

			layout (location = 0) in vec3 a_Position;
			layout (location = 1) in vec3 a_Color;
			
			uniform mat4 u_ViewProjectionMatrix;

			out vec4 Color;
			
			void main()
			{
				Color = vec4(a_Color, 1);
				gl_Position = u_ViewProjectionMatrix * vec4(a_Position, 1);
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

		m_Shader = std::make_unique<Athena::Shader>(vertexSrc, fragmentSrc);
	}

	void OnUpdate(Athena::Time frameTime) override
	{
		if (Athena::Input::IsKeyPressed(Athena::Key::A))
			m_CameraPosition.x -= m_CameraSpeed * frameTime.AsSeconds();
		else if (Athena::Input::IsKeyPressed(Athena::Key::D))
			m_CameraPosition.x += m_CameraSpeed * frameTime.AsSeconds();
		else if (Athena::Input::IsKeyPressed(Athena::Key::W))
			m_CameraPosition.y += m_CameraSpeed * frameTime.AsSeconds();
		else if (Athena::Input::IsKeyPressed(Athena::Key::S ))
			m_CameraPosition.y -= m_CameraSpeed * frameTime.AsSeconds();

		Athena::RenderCommand::Clear({ 0.1f, 0.1f, 0.1f, 1 });

		m_CameraRotation += 0.5f;
		m_Camera.SetRotation(m_CameraRotation);
		m_Camera.SetPosition(m_CameraPosition);

		Athena::Renderer::BeginScene(m_Camera);

		Athena::Renderer::Submit(m_Shader, m_VertexArray);

		Athena::Renderer::EndScene();
	}

	void OnImGuiRender() override
	{

	}

	void OnEvent(Athena::Event& event) override
	{

	}

private:
	std::shared_ptr<Athena::Shader> m_Shader;
	std::shared_ptr<Athena::VertexArray> m_VertexArray;

	Athena::OrthographicCamera m_Camera;
	Athena::Vector3 m_CameraPosition;
	float m_CameraRotation = 0;
	float m_CameraSpeed = 2.f;
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
