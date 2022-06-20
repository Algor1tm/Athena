#include "atnpch.h"
#include "Renderer2D.h"

#include "RenderCommand.h"
#include "Shader.h"

#include "Athena/Platform/OpenGL/OpenGLShader.h"


namespace Athena
{
	struct Renderer2DStorage
	{
		Ref<VertexArray> QuadVertexArray;
		Ref<Shader> FlatColorShader;
	};

	static Renderer2DStorage* s_Data;

	void Renderer2D::Init()
	{
		s_Data = new Renderer2DStorage();

		float vertices[] = {
			-0.5, -0.5f, 0.0f,
			0.5f, -0.5f, 0.0f,
			0.5f, 0.5f, 0.0f,
			-0.5f, 0.5f, 0.0f
		};

		Ref<VertexBuffer> vertexBuffer;
		vertexBuffer = VertexBuffer::Create(vertices, (uint32_t)std::size(vertices));

		BufferLayout layout = { {ShaderDataType::Float3, "a_Position"}, };
		vertexBuffer->SetLayout(layout);


		Ref<IndexBuffer> indexBuffer;
		unsigned int indices[6] = { 0, 1, 2, 2, 3, 0 };
		indexBuffer = IndexBuffer::Create(indices, (uint32_t)std::size(indices));

		s_Data->QuadVertexArray = VertexArray::Create();
		s_Data->QuadVertexArray->AddVertexBuffer(vertexBuffer);
		s_Data->QuadVertexArray->SetIndexBuffer(indexBuffer);
			  
		s_Data->FlatColorShader = Shader::Create("assets/shaders/FlatColor.glsl");
	}

	void Renderer2D::Shutdown()
	{

	}

	void Renderer2D::BeginScene(const OrthographicCamera& camera)
	{
		std::dynamic_pointer_cast<OpenGLShader>(s_Data->FlatColorShader)->Bind();
		std::dynamic_pointer_cast<OpenGLShader>(s_Data->FlatColorShader)->UploadUniformMat4(
			"u_ViewProjection", camera.GetViewProjectionMatrix());
		std::dynamic_pointer_cast<OpenGLShader>(s_Data->FlatColorShader)->UploadUniformMat4(
			"u_Transform", Matrix4::Identity());
	}

	void Renderer2D::EndScene()
	{

	}

	void Renderer2D::DrawQuad(const Vector2& position, const Vector2& size, const Color& color)
	{
		DrawQuad({ position.x, position.y, 0.f }, size, color);
	}

	void Renderer2D::DrawQuad(const Vector3& position, const Vector2& size, const Color& color)
	{
		std::dynamic_pointer_cast<OpenGLShader>(s_Data->FlatColorShader)->Bind();
		std::dynamic_pointer_cast<Athena::OpenGLShader>(s_Data->FlatColorShader)->UploadUniformFloat4(
			"u_Color", color);

		s_Data->QuadVertexArray->Bind();
		RenderCommand::DrawIndexed(s_Data->QuadVertexArray);
	}
}
