#include "atnpch.h"
#include "Renderer2D.h"

#include "RenderCommand.h"
#include "Shader.h"


namespace Athena
{
	struct QuadVertex
	{
		Vector3 Position;
		Color Color;	// Vector<4, float> has issues with allignment
		Vector2 TexCoord;
		float TexIndex;
		float TilingFactor;
	};

	struct Renderer2DData
	{
		static const uint32_t MaxQuads = 10000;
		static const uint32_t MaxVertices = MaxQuads * 4;
		static const uint32_t MaxIndices = MaxQuads * 6;
		static const uint32_t MaxTextureSlots = 32;   // TODO: RenderCaps

		Ref<VertexArray> QuadVertexArray;
		Ref<VertexBuffer> QuadVertexBuffer;
		Ref<Shader> TextureShader;
		Ref<Texture2D> WhiteTexture;

		uint32_t QuadIndexCount = 0;
		QuadVertex* QuadVertexBufferBase = nullptr;
		QuadVertex* QuadVertexBufferPointer = nullptr;

		std::array<Ref<Texture2D>, MaxTextureSlots> TextureSlots;
		uint32_t TextureSlotIndex = 1; // 0 - white texture

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
			{ShaderDataType::Float, "a_TexIndex"},
			{ShaderDataType::Float, "a_TilingFactor"}
		};
		s_Data.QuadVertexBuffer->SetLayout(layout);
		s_Data.QuadVertexArray->AddVertexBuffer(s_Data.QuadVertexBuffer);

		s_Data.QuadVertexBufferBase = new QuadVertex[Renderer2DData::MaxVertices];

		uint32_t* quadIndices = new uint32_t[Renderer2DData::MaxIndices];

		uint32_t offset = 0;
		for (uint32_t i = 0; i < Renderer2DData::MaxIndices; i += 6)
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

		s_Data.WhiteTexture = Texture2D::Create(1, 1);
		uint32_t whiteTextureData = 0xffffffff;
		s_Data.WhiteTexture->SetData(&whiteTextureData, sizeof(uint32_t));

		int32_t samplers[Renderer2DData::MaxTextureSlots];
		for (int32_t i = 0; i < std::size(samplers); ++i)
			samplers[i] = i;

		s_Data.TextureShader = Shader::Create("assets/shaders/Texture.glsl");
		s_Data.TextureShader->Bind();
		s_Data.TextureShader->SetIntArray("u_Texture", samplers, s_Data.MaxTextureSlots);

		s_Data.TextureSlots[0] = s_Data.WhiteTexture;

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

	void Renderer2D::BeginScene(const Camera& camera)
	{
		ATN_PROFILE_FUNCTION();

		s_Data.TextureShader->Bind();
		s_Data.TextureShader->SetMat4("u_ViewProjection", camera.GetViewProjectionMatrix());

		StartBatch();
	}

	void Renderer2D::EndScene()
	{
		ATN_PROFILE_FUNCTION();

		uint64_t dataSize = (uint8_t*)s_Data.QuadVertexBufferPointer - (uint8_t*)s_Data.QuadVertexBufferBase;
		s_Data.QuadVertexBuffer->SetData(s_Data.QuadVertexBufferBase, (uint32_t)dataSize);

		Flush();
	}

	void Renderer2D::Flush()
	{
		ATN_PROFILE_FUNCTION();

		for (uint32_t i = 0; i < s_Data.TextureSlotIndex; ++i)
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

	void Renderer2D::DrawQuad(const Vector2& position, const Vector2& size, const Color& color)
	{
		DrawQuad({ position.x, position.y, 0.f }, size, color);
	}

	void Renderer2D::DrawQuad(const Vector3& position, const Vector2& size, const Color& color)
	{
		ATN_PROFILE_FUNCTION();

		if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
			NextBatch();
		
		constexpr size_t QuadVertexCount = 4;
		constexpr float textureIndex = 0.f; // White Texture
		constexpr Vector2 textureCoords[] = { {0.f, 0.f}, {1.f, 0.f}, {1.f, 1.f}, {0.f, 1.f} };
		constexpr float tilingFactor = 1.f;

		Matrix4 transform = Scale({ size.x, size.y, 1.f }) * Translate(position);

		for (size_t i = 0; i < QuadVertexCount; ++i)
		{
			s_Data.QuadVertexBufferPointer->Position = s_Data.QuadVertexPositions[i] * transform;
			s_Data.QuadVertexBufferPointer->Color = color;
			s_Data.QuadVertexBufferPointer->TexCoord = textureCoords[i];
			s_Data.QuadVertexBufferPointer->TexIndex = textureIndex;
			s_Data.QuadVertexBufferPointer->TilingFactor = tilingFactor;
			s_Data.QuadVertexBufferPointer++;
		}

		s_Data.QuadIndexCount += 6;

		s_Data.Stats.QuadCount++;
	}

	void Renderer2D::DrawQuad(const Vector2& position, const Vector2& size, const Ref<Texture2D>& texture, float tilingFactor, const Color& tint)
	{
		DrawQuad({ position.x, position.y, 0.f }, size, texture, tilingFactor, tint);
	}

	void Renderer2D::DrawQuad(const Vector3& position, const Vector2& size, const Ref<Texture2D>& texture, float tilingFactor, const Color& tint)
	{
		ATN_PROFILE_FUNCTION();

		if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
			NextBatch();

		constexpr size_t QuadVertexCount = 4;
		constexpr Vector2 textureCoords[] = { {0.f, 0.f}, {1.f, 0.f}, {1.f, 1.f}, {0.f, 1.f} };
		float textureIndex = 0.0f;

		for (uint32_t i = 1; i < s_Data.TextureSlotIndex; ++i)
		{
			if (*s_Data.TextureSlots[i] == *texture)
			{
				textureIndex = 0;
				break;
			}
		}

		if (textureIndex == 0.0f)
		{
			textureIndex = (float)s_Data.TextureSlotIndex;
			s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
			s_Data.TextureSlotIndex++;
		}

		Matrix4 transform = Scale({ size.x, size.y, 1.f }) * Translate(position);

		for (size_t i = 0; i < QuadVertexCount; ++i)
		{
			s_Data.QuadVertexBufferPointer->Position = s_Data.QuadVertexPositions[i] * transform;
			s_Data.QuadVertexBufferPointer->Color = tint;
			s_Data.QuadVertexBufferPointer->TexCoord = textureCoords[i];
			s_Data.QuadVertexBufferPointer->TexIndex = textureIndex;
			s_Data.QuadVertexBufferPointer->TilingFactor = tilingFactor;
			s_Data.QuadVertexBufferPointer++;
		}

		s_Data.QuadIndexCount += 6;

		s_Data.Stats.QuadCount++;
	}

	void Renderer2D::DrawQuad(const Vector2& position, const Vector2& size, const Ref<SubTexture2D>& subtexture, float tilingFactor, const Color& tint)
	{
		DrawQuad({ position.x, position.y, 0.f }, size, subtexture, tilingFactor, tint);
	}

	void Renderer2D::DrawQuad(const Vector3& position, const Vector2& size, const Ref<SubTexture2D>& subtexture, float tilingFactor, const Color& tint)
	{
		ATN_PROFILE_FUNCTION();

		if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
			NextBatch();

		constexpr size_t QuadVertexCount = 4;
		const Vector2* textureCoords = subtexture->GetTexCoords();
		float textureIndex = 0.0f;
		const Ref<Texture2D>& texture = subtexture->GetTexture();

		for (uint32_t i = 1; i < s_Data.TextureSlotIndex; ++i)
		{
			if (*s_Data.TextureSlots[i] == *texture)
			{
				textureIndex = 0;
				break;
			}
		}

		if (textureIndex == 0.0f)
		{
			textureIndex = (float)s_Data.TextureSlotIndex;
			s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
			s_Data.TextureSlotIndex++;
		}

		Matrix4 transform = Scale({ size.x, size.y, 1.f }) * Translate(position);

		for (size_t i = 0; i < QuadVertexCount; ++i)
		{
			s_Data.QuadVertexBufferPointer->Position = s_Data.QuadVertexPositions[i] * transform;
			s_Data.QuadVertexBufferPointer->Color = tint;
			s_Data.QuadVertexBufferPointer->TexCoord = textureCoords[i];
			s_Data.QuadVertexBufferPointer->TexIndex = textureIndex;
			s_Data.QuadVertexBufferPointer->TilingFactor = tilingFactor;
			s_Data.QuadVertexBufferPointer++;
		}

		s_Data.QuadIndexCount += 6;

		s_Data.Stats.QuadCount++;
	}

	void Renderer2D::DrawRotatedQuad(const Vector2& position, const Vector2& size, float rotation, const Color& color)
	{
		DrawRotatedQuad({ position.x, position.y, 0.f }, size, rotation, color);
	}

	void Renderer2D::DrawRotatedQuad(const Vector3& position, const Vector2& size, float rotation, const Color& color)
	{
		ATN_PROFILE_FUNCTION();

		if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
			NextBatch();

		constexpr size_t QuadVertexCount = 4;
		constexpr float textureIndex = 0.f; // White Texture
		constexpr Vector2 textureCoords[] = { {0.f, 0.f}, {1.f, 0.f}, {1.f, 1.f}, {0.f, 1.f} };
		constexpr float tilingFactor = 1.f;

		Matrix4 transform =
			Scale({ size.x, size.y, 1.f }) *
			Rotate(rotation, { 0.f, 0.f, 1.f }) *
			Translate(position);

		for (size_t i = 0; i < QuadVertexCount; ++i)
		{
			s_Data.QuadVertexBufferPointer->Position = s_Data.QuadVertexPositions[i] * transform;
			s_Data.QuadVertexBufferPointer->Color = color;
			s_Data.QuadVertexBufferPointer->TexCoord = textureCoords[i];
			s_Data.QuadVertexBufferPointer->TexIndex = textureIndex;
			s_Data.QuadVertexBufferPointer->TilingFactor = tilingFactor;
			s_Data.QuadVertexBufferPointer++;
		}

		s_Data.QuadIndexCount += 6;

		s_Data.Stats.QuadCount++;
	}

	void Renderer2D::DrawRotatedQuad(const Vector2& position, const Vector2& size, float rotation, const Ref<Texture2D>& texture, float tilingFactor, const Color& tint)
	{
		DrawRotatedQuad({ position.x, position.y, 0.f }, size, rotation, texture, tilingFactor, tint);
	}

	void Renderer2D::DrawRotatedQuad(const Vector3& position, const Vector2& size, float rotation, const Ref<Texture2D>& texture, float tilingFactor, const Color& tint)
	{
		ATN_PROFILE_FUNCTION();

		if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
			NextBatch();

		constexpr size_t QuadVertexCount = 4;
		constexpr Vector2 textureCoords[] = { {0.f, 0.f}, {1.f, 0.f}, {1.f, 1.f}, {0.f, 1.f} };
		float textureIndex = 0.0f;

		for (uint32_t i = 1; i < s_Data.TextureSlotIndex; ++i)
		{
			if (*s_Data.TextureSlots[i] == *texture)
			{
				textureIndex = 0;
				break;
			}
		}

		if (textureIndex == 0.0f)
		{
			textureIndex = (float)s_Data.TextureSlotIndex;
			s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
			s_Data.TextureSlotIndex++;
		}

		Matrix4 transform = 
			Scale({ size.x, size.y, 1.f }) *
			Rotate(rotation, { 0.f, 0.f, 1.f }) *
			Translate(position);

		for (size_t i = 0; i < QuadVertexCount; ++i)
		{
			s_Data.QuadVertexBufferPointer->Position = s_Data.QuadVertexPositions[i] * transform;
			s_Data.QuadVertexBufferPointer->Color = tint;
			s_Data.QuadVertexBufferPointer->TexCoord = textureCoords[i];
			s_Data.QuadVertexBufferPointer->TexIndex = textureIndex;
			s_Data.QuadVertexBufferPointer->TilingFactor = tilingFactor;
			s_Data.QuadVertexBufferPointer++;
		}

		s_Data.QuadIndexCount += 6;

		s_Data.Stats.QuadCount++;
	}

	void Renderer2D::DrawRotatedQuad(const Vector2& position, const Vector2& size, float rotation, const Ref<SubTexture2D>& subtexture, float tilingFactor, const Color& tint)
	{
		DrawRotatedQuad({ position.x, position.y, 0.f }, size, rotation, subtexture, tilingFactor, tint);
	}

	void Renderer2D::DrawRotatedQuad(const Vector3& position, const Vector2& size, float rotation, const Ref<SubTexture2D>& subtexture, float tilingFactor, const Color& tint)
	{
		ATN_PROFILE_FUNCTION();

		if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
			NextBatch();

		constexpr size_t QuadVertexCount = 4;
		const Vector2* textureCoords = subtexture->GetTexCoords();
		float textureIndex = 0.0f;
		const Ref<Texture2D>& texture = subtexture->GetTexture();

		for (uint32_t i = 1; i < s_Data.TextureSlotIndex; ++i)
		{
			if (*s_Data.TextureSlots[i] == *texture)
			{
				textureIndex = 0;
				break;
			}
		}

		if (textureIndex == 0.0f)
		{
			textureIndex = (float)s_Data.TextureSlotIndex;
			s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
			s_Data.TextureSlotIndex++;
		}

		Matrix4 transform =
			Scale({ size.x, size.y, 1.f }) *
			Rotate(rotation, { 0.f, 0.f, 1.f }) *
			Translate(position);

		for (size_t i = 0; i < QuadVertexCount; ++i)
		{
			s_Data.QuadVertexBufferPointer->Position = s_Data.QuadVertexPositions[i] * transform;
			s_Data.QuadVertexBufferPointer->Color = tint;
			s_Data.QuadVertexBufferPointer->TexCoord = textureCoords[i];
			s_Data.QuadVertexBufferPointer->TexIndex = textureIndex;
			s_Data.QuadVertexBufferPointer->TilingFactor = tilingFactor;
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
