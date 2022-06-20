#include "atnpch.h"
#include "Renderer2D.h"

#include "RenderCommand.h"
#include "Shader.h"


namespace Athena
{
	struct Renderer2DStorage
	{
		Ref<VertexArray> QuadVertexArray;
		Ref<Shader> TextureShader;
		Ref<Texture> WhiteTexture;
	};

	static Renderer2DStorage* s_Data;

	void Renderer2D::Init()
	{
		s_Data = new Renderer2DStorage();

		float vertices[] = {
			-0.5, -0.5f, 0.0f,   0.0f, 0.0f,
			0.5f, -0.5f, 0.0f,   1.f, 0.f,
			0.5f, 0.5f, 0.0f,    1.f, 1.f, 
			-0.5f, 0.5f, 0.0f,   0.f, 1.f
		};

		Ref<VertexBuffer> vertexBuffer;
		vertexBuffer = VertexBuffer::Create(vertices, (uint32_t)std::size(vertices));

		BufferLayout layout = { {ShaderDataType::Float3, "a_Position"},
								{ShaderDataType::Float2, "a_TexCoord"} };
		vertexBuffer->SetLayout(layout);


		Ref<IndexBuffer> indexBuffer;
		unsigned int indices[6] = { 0, 1, 2, 2, 3, 0 };
		indexBuffer = IndexBuffer::Create(indices, (uint32_t)std::size(indices));

		s_Data->QuadVertexArray = VertexArray::Create();
		s_Data->QuadVertexArray->AddVertexBuffer(vertexBuffer);
		s_Data->QuadVertexArray->SetIndexBuffer(indexBuffer);
		
		s_Data->WhiteTexture = Texture2D::Create(1, 1);
		uint32_t whiteTextureData = 0xffffffff;
		s_Data->WhiteTexture->SetData(&whiteTextureData, sizeof(uint32_t));

		s_Data->TextureShader = Shader::Create("assets/shaders/Texture.glsl");
		s_Data->TextureShader->Bind();
		s_Data->TextureShader->SetInt("u_Texture", 0);
	}

	void Renderer2D::Shutdown()
	{

	}

	void Renderer2D::BeginScene(const OrthographicCamera& camera)
	{
		s_Data->TextureShader->Bind();
		s_Data->TextureShader->SetMat4("u_ViewProjection", camera.GetViewProjectionMatrix());
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
		s_Data->TextureShader->SetFloat4("u_Color", color);
		s_Data->WhiteTexture->Bind();

		Matrix4 transform = Scale({ size.x, size.y, 1.f }) * Translate(position);
		s_Data->TextureShader->SetMat4("u_Transform", transform);

		s_Data->QuadVertexArray->Bind();
		RenderCommand::DrawIndexed(s_Data->QuadVertexArray);
	}

	void Renderer2D::DrawQuad(const Vector2& position, const Vector2& size, const Ref<Texture2D>& texture)
	{
		DrawQuad({ position.x, position.y, 0.f }, size, texture);
	}

	void Renderer2D::DrawQuad(const Vector3& position, const Vector2& size, const Ref<Texture2D>& texture)
	{
		s_Data->TextureShader->SetFloat4("u_Color", Vector4(1));
		texture->Bind();

		Matrix4 transform = Scale({ size.x, size.y, 1.f }) * Translate(position);
		s_Data->TextureShader->SetMat4("u_Transform", transform);

		s_Data->QuadVertexArray->Bind();
		RenderCommand::DrawIndexed(s_Data->QuadVertexArray);
	}
}
