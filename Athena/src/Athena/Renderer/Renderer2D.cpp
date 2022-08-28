#include "atnpch.h"
#include "Renderer2D.h"

#include "RenderCommand.h"
#include "Shader.h"


namespace Athena
{
	struct QuadVertex
	{
		Vector3 Translation;
		LinearColor Color;
		Vector2 TexCoord;
		float TexIndex;
		float TilingFactor;

		int EntityID = 0;
	};

	struct Renderer2DData
	{
		static const uint32 MaxQuads = 500;
		static const uint32 MaxVertices = MaxQuads * 4;
		static const uint32 MaxIndices = MaxQuads * 6;
		static const uint32 MaxTextureSlots = 32;   // TODO: RenderCaps

		Ref<VertexArray> QuadVertexArray;
		Ref<VertexBuffer> QuadVertexBuffer;
		Ref<Shader> TextureShader;

		uint32 QuadIndexCount = 0;
		QuadVertex* QuadVertexBufferBase = nullptr;
		QuadVertex* QuadVertexBufferPointer = nullptr;

		std::array<Ref<Texture2D>, MaxTextureSlots> TextureSlots;
		uint32 TextureSlotIndex = 1; // 0 - white texture

		Vector4 QuadVertexPositions[4];

		Renderer2D::Statistics Stats;
	};

	static Renderer2DData s_Data;

	void Renderer2D::Init()
	{
		ATN_PROFILE_FUNCTION();
		s_Data.QuadVertexArray = VertexArray::Create();

		s_Data.QuadVertexBuffer = VertexBuffer::Create(Renderer2DData::MaxVertices * sizeof(QuadVertex));
		BufferLayout layout = 
		{ 
			{ShaderDataType::Float3, "a_Position"},
			{ShaderDataType::Float4, "a_Color"},
			{ShaderDataType::Float2, "a_TexCoord"},
			{ShaderDataType::Float,  "a_TexIndex"},
			{ShaderDataType::Float,  "a_TilingFactor"},
			{ShaderDataType::Int,    "a_EntityID"}
		};
		s_Data.QuadVertexBuffer->SetLayout(layout);
		s_Data.QuadVertexArray->AddVertexBuffer(s_Data.QuadVertexBuffer);

		s_Data.QuadVertexBufferBase = new QuadVertex[Renderer2DData::MaxVertices];

		uint32* quadIndices = new uint32[Renderer2DData::MaxIndices];

		uint32 offset = 0;
		for (uint32 i = 0; i < Renderer2DData::MaxIndices; i += 6)
		{
			quadIndices[i + 0] = offset + 0;
			quadIndices[i + 1] = offset + 1;
			quadIndices[i + 2] = offset + 2;

			quadIndices[i + 3] = offset + 2;
			quadIndices[i + 4] = offset + 3;
			quadIndices[i + 5] = offset + 0;

			offset += 4;
		}

		Ref<IndexBuffer> indexBuffer = IndexBuffer::Create(quadIndices, Renderer2DData::MaxIndices);
		s_Data.QuadVertexArray->SetIndexBuffer(indexBuffer);
		delete[] quadIndices;

		int32 samplers[Renderer2DData::MaxTextureSlots];
		for (int32 i = 0; i < std::size(samplers); ++i)
			samplers[i] = i;

		s_Data.TextureShader = Shader::Create("assets/shaders/Texture.glsl");
		s_Data.TextureShader->Bind();
		s_Data.TextureShader->SetIntArray("u_Texture", samplers, s_Data.MaxTextureSlots);

		s_Data.TextureSlots[0] = Texture2D::WhiteTexture();

		s_Data.QuadVertexPositions[0] = { -0.5f, -0.5f, 0.f, 1.f };
		s_Data.QuadVertexPositions[1] = { 0.5f, -0.5f, 0.f, 1.f };
		s_Data.QuadVertexPositions[2] = { 0.5f, 0.5f, 0.f, 1.f };
		s_Data.QuadVertexPositions[3] = { -0.5f, 0.5f, 0.f, 1.f };
	}

	void Renderer2D::Shutdown()
	{
		ATN_PROFILE_FUNCTION();

		delete[] s_Data.QuadVertexBufferBase;
	}

	void Renderer2D::BeginScene(const Camera& camera, const Matrix4& transform)
	{
		ATN_PROFILE_FUNCTION();

		Matrix4 viewProj = AffineInverse(transform) * camera.GetProjection();

		s_Data.TextureShader->Bind();
		s_Data.TextureShader->SetMat4("u_ViewProjection", viewProj);

		StartBatch();
	}

	void Renderer2D::BeginScene(const OrthographicCamera& camera)
	{
		ATN_PROFILE_FUNCTION();

		s_Data.TextureShader->Bind();
		s_Data.TextureShader->SetMat4("u_ViewProjection", camera.GetViewProjectionMatrix());

		StartBatch();
	}

	void Renderer2D::BeginScene(const EditorCamera& camera)
	{
		ATN_PROFILE_FUNCTION();

		s_Data.TextureShader->Bind();
		s_Data.TextureShader->SetMat4("u_ViewProjection", camera.GetViewProjection());

		StartBatch();
	}

	void Renderer2D::EndScene()
	{
		ATN_PROFILE_FUNCTION();

		uint64 dataSize = (byte*)s_Data.QuadVertexBufferPointer - (byte*)s_Data.QuadVertexBufferBase;
		s_Data.QuadVertexBuffer->SetData(s_Data.QuadVertexBufferBase, (uint32)dataSize);

		Flush();
	}

	void Renderer2D::Flush()
	{
		ATN_PROFILE_FUNCTION();

		for (uint32 i = 0; i < s_Data.TextureSlotIndex; ++i)
			s_Data.TextureSlots[i]->Bind(i);

		RenderCommand::DrawIndexed(s_Data.QuadVertexArray, s_Data.QuadIndexCount);
		s_Data.Stats.DrawCalls++;
	}

	void Renderer2D::StartBatch()
	{
		s_Data.QuadIndexCount = 0;
		s_Data.QuadVertexBufferPointer = s_Data.QuadVertexBufferBase;

		s_Data.TextureSlotIndex = 1;
	}

	void Renderer2D::NextBatch()
	{
		EndScene();
		StartBatch();
	}

	void Renderer2D::DrawQuad(const Vector2& position, const Vector2& size, const LinearColor& color)
	{
		DrawQuad({ position.x, position.y, 0.f }, size, color);
	}

	void Renderer2D::DrawQuad(const Vector3& position, const Vector2& size, const LinearColor& color)
	{
		Matrix4 transform = ScaleMatrix(Vector3( size.x, size.y, 1.f )).Translate(position);

		DrawQuad(transform, color);
	}

	void Renderer2D::DrawQuad(const Vector2& position, const Vector2& size, const Texture2DInstance& texture, const LinearColor& tint, float tilingFactor)
	{
		DrawQuad({ position.x, position.y, 0.f }, size, texture, tint, tilingFactor);
	}

	void Renderer2D::DrawQuad(const Vector3& position, const Vector2& size, const Texture2DInstance& texture, const LinearColor& tint, float tilingFactor)
	{
		Matrix4 transform = ScaleMatrix(Vector3(size.x, size.y, 1.f )).Translate(position);
		
		DrawQuad(transform, texture, tint, tilingFactor);
	}

	void Renderer2D::DrawRotatedQuad(const Vector2& position, const Vector2& size, float rotation, const LinearColor& color)
	{
		DrawRotatedQuad({ position.x, position.y, 0.f }, size, rotation, color);
	}

	void Renderer2D::DrawRotatedQuad(const Vector3& position, const Vector2& size, float rotation, const LinearColor& color)
	{
		Matrix4 transform =
			ScaleMatrix(Vector3(size.x, size.y, 1.f)).Rotate(rotation, Vector3(0.f, 0.f, 1.f)).Translate(position);

		DrawQuad(transform, color);
	}

	void Renderer2D::DrawRotatedQuad(const Vector2& position, const Vector2& size, float rotation, const Texture2DInstance& texture, const LinearColor& tint, float tilingFactor)
	{
		DrawRotatedQuad({ position.x, position.y, 0.f }, size, rotation, texture, tint, tilingFactor);
	}

	void Renderer2D::DrawRotatedQuad(const Vector3& position, const Vector2& size, float rotation, const Texture2DInstance& texture, const LinearColor& tint, float tilingFactor)
	{
		Matrix4 transform = 
			ScaleMatrix(Vector3(size.x, size.y, 1.f)).Rotate(rotation, Vector3(0.f, 0.f, 1.f)).Translate(position);

		DrawQuad(transform, texture, tint, tilingFactor);
	}

	void Renderer2D::DrawQuad(const Matrix4& transform, const LinearColor& color, int entityID)
	{
		ATN_PROFILE_FUNCTION();

		if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
			NextBatch();

		constexpr SIZE_T QuadVertexCount = 4;
		constexpr float textureIndex = 0.f; // White Texture
		constexpr Vector2 textureCoords[4] = { {0.f, 0.f}, {1.f, 0.f}, {1.f, 1.f}, {0.f, 1.f} };
		constexpr float tilingFactor = 1.f;

		for (SIZE_T i = 0; i < QuadVertexCount; ++i)
		{
			s_Data.QuadVertexBufferPointer->Translation = s_Data.QuadVertexPositions[i] * transform;
			s_Data.QuadVertexBufferPointer->Color = color;
			s_Data.QuadVertexBufferPointer->TexCoord = textureCoords[i];
			s_Data.QuadVertexBufferPointer->TexIndex = textureIndex;
			s_Data.QuadVertexBufferPointer->TilingFactor = tilingFactor;
			s_Data.QuadVertexBufferPointer->EntityID = entityID;
			s_Data.QuadVertexBufferPointer++;
		}

		s_Data.QuadIndexCount += 6;

		s_Data.Stats.QuadCount++;
	}

	void Renderer2D::DrawQuad(const Matrix4& transform, const Texture2DInstance& texture, const LinearColor& tint, float tilingFactor, int entityID)
	{
		ATN_PROFILE_FUNCTION();

		if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
			NextBatch();

		constexpr SIZE_T QuadVertexCount = 4;
		const auto& texCoords = texture.GetTexCoords();
		float textureIndex = 0.0f;

		for (uint32 i = 1; i < s_Data.TextureSlotIndex; ++i)
		{
			if (*s_Data.TextureSlots[i] == *(texture.GetNativeTexture()))
			{
				textureIndex = (float)i;
				break;
			}
		}

		if (textureIndex == 0.0f)
		{
			if (s_Data.TextureSlotIndex >= Renderer2DData::MaxTextureSlots)
				NextBatch();

			textureIndex = (float)s_Data.TextureSlotIndex;
			s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture.GetNativeTexture();
			s_Data.TextureSlotIndex++;
		}

		for (SIZE_T i = 0; i < QuadVertexCount; ++i)
		{
			s_Data.QuadVertexBufferPointer->Translation = s_Data.QuadVertexPositions[i] * transform;
			s_Data.QuadVertexBufferPointer->Color = tint;
			s_Data.QuadVertexBufferPointer->TexCoord = texCoords[i];
			s_Data.QuadVertexBufferPointer->TexIndex = textureIndex;
			s_Data.QuadVertexBufferPointer->TilingFactor = tilingFactor;
			s_Data.QuadVertexBufferPointer->EntityID = entityID;
			s_Data.QuadVertexBufferPointer++;
		}

		s_Data.QuadIndexCount += 6;

		s_Data.Stats.QuadCount++;
	}


	void Renderer2D::ResetStats()
	{
		s_Data.Stats.DrawCalls = 0;
		s_Data.Stats.QuadCount = 0;
	}

	Renderer2D::Statistics Renderer2D::GetStats()
	{
		return s_Data.Stats;
	}
}
