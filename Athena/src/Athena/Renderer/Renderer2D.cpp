#include "atnpch.h"
#include "Renderer2D.h"

#include "RenderCommand.h"
#include "Shader.h"


namespace Athena
{
	struct QuadVertex
	{
		Vector3 Position;
		LinearColor Color;
		Vector2 TexCoord;
		float TexIndex;
		float TilingFactor;

		int EntityID = 0;
	};

	struct CircleVertex
	{
		Vector3 WorldPosition;
		Vector3 LocalPosition;
		LinearColor Color;
		float Thickness;
		float Fade;

		int EntityID = 0;
	};

	struct LineVertex
	{
		Vector3 Position;
		LinearColor Color;

		int EntityID = 0;
	};

	struct Renderer2DData
	{
		static const uint32 MaxQuads = 500;
		static const uint32 MaxQuadVertices = MaxQuads * 4;
		static const uint32 MaxCircles = 300;
		static const uint32 MaxCircleVertices = MaxCircles * 4;
		static const uint32 MaxLines = 300;
		static const uint32 MaxLineVertices = MaxLines * 2;
		static const uint32 MaxIndices = Math::Max(MaxQuads, MaxCircles, MaxLines) * 6; // shared indices

		static const uint32 MaxTextureSlots = 32;   // TODO: RenderCaps

		Ref<VertexArray> QuadVertexArray;
		Ref<VertexBuffer> QuadVertexBuffer;
		Ref<Shader> QuadShader;

		Ref<VertexArray> CircleVertexArray;
		Ref<VertexBuffer> CircleVertexBuffer;
		Ref<Shader> CircleShader;

		Ref<VertexArray> LineVertexArray;
		Ref<VertexBuffer> LineVertexBuffer;
		Ref<Shader> LineShader;

		uint32 QuadIndexCount = 0;
		QuadVertex* QuadVertexBufferBase = nullptr;
		QuadVertex* QuadVertexBufferPointer = nullptr;

		uint32 CircleIndexCount = 0;
		CircleVertex* CircleVertexBufferBase = nullptr;
		CircleVertex* CircleVertexBufferPointer = nullptr;

		uint32 LineVertexCount = 0;
		LineVertex* LineVertexBufferBase = nullptr;
		LineVertex* LineVertexBufferPointer = nullptr;

		float LineWidth = 2.0f;

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
		s_Data.QuadVertexBuffer = VertexBuffer::Create(Renderer2DData::MaxQuadVertices * sizeof(QuadVertex));
		s_Data.QuadVertexBuffer->SetLayout({
			{ ShaderDataType::Float3, "a_Position"     },
			{ ShaderDataType::Float4, "a_Color"        },
			{ ShaderDataType::Float2, "a_TexCoord"     },
			{ ShaderDataType::Float,  "a_TexIndex"     },
			{ ShaderDataType::Float,  "a_TilingFactor" },
			{ ShaderDataType::Int,    "a_EntityID"     }
			});
		s_Data.QuadVertexArray->AddVertexBuffer(s_Data.QuadVertexBuffer);
		s_Data.QuadVertexBufferBase = new QuadVertex[Renderer2DData::MaxQuadVertices];

		uint32* indices = new uint32[Renderer2DData::MaxIndices];

		uint32 offset = 0;
		for (uint32 i = 0; i < Renderer2DData::MaxIndices; i += 6)
		{
			indices[i + 0] = offset + 0;
			indices[i + 1] = offset + 1;
			indices[i + 2] = offset + 2;

			indices[i + 3] = offset + 2;
			indices[i + 4] = offset + 3;
			indices[i + 5] = offset + 0;

			offset += 4;
		}

		Ref<IndexBuffer> indexBuffer = IndexBuffer::Create(indices, Renderer2DData::MaxIndices);
		s_Data.QuadVertexArray->SetIndexBuffer(indexBuffer);
		delete[] indices;

		s_Data.CircleVertexArray = VertexArray::Create();
		s_Data.CircleVertexBuffer = VertexBuffer::Create(Renderer2DData::MaxCircles * sizeof(CircleVertex));
		s_Data.CircleVertexBuffer->SetLayout({
			{ ShaderDataType::Float3, "a_WorldPosition"  },
			{ ShaderDataType::Float3, "a_LocalPosition"  },
			{ ShaderDataType::Float4, "a_Color"     },
			{ ShaderDataType::Float,  "a_Thickness" },
			{ ShaderDataType::Float,  "a_Fade"      },
			{ ShaderDataType::Int,    "a_EntityID"  }
			});
		s_Data.CircleVertexArray->AddVertexBuffer(s_Data.CircleVertexBuffer);
		s_Data.CircleVertexArray->SetIndexBuffer(indexBuffer); // quad index buffer
		s_Data.CircleVertexBufferBase = new CircleVertex[Renderer2DData::MaxCircleVertices];

		s_Data.LineVertexArray = VertexArray::Create();
		s_Data.LineVertexBuffer = VertexBuffer::Create(Renderer2DData::MaxCircles * sizeof(LineVertex));
		s_Data.LineVertexBuffer->SetLayout({
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float4, "a_Color"    },
			{ ShaderDataType::Int,    "a_EntityID" }
			});
		s_Data.LineVertexArray->AddVertexBuffer(s_Data.LineVertexBuffer);
		s_Data.LineVertexArray->SetIndexBuffer(indexBuffer); // quad index buffer
		s_Data.LineVertexBufferBase = new LineVertex[Renderer2DData::MaxLineVertices];

		s_Data.QuadShader = Shader::Create("assets/shaders/Quad.glsl");
		s_Data.CircleShader = Shader::Create("assets/shaders/Circle.glsl");
		s_Data.LineShader = Shader::Create("assets/shaders/Line.glsl");

		int32 samplers[Renderer2DData::MaxTextureSlots];
		for (int32 i = 0; i < std::size(samplers); ++i)
			samplers[i] = i;

		s_Data.QuadShader->Bind();
		s_Data.QuadShader->SetIntArray("u_Texture", samplers, s_Data.MaxTextureSlots);
		s_Data.QuadShader->UnBind();

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

		s_Data.QuadShader->Bind();
		s_Data.QuadShader->SetMat4("u_ViewProjection", viewProj);

		s_Data.CircleShader->Bind();
		s_Data.CircleShader->SetMat4("u_ViewProjection", viewProj);

		s_Data.LineShader->Bind();
		s_Data.LineShader->SetMat4("u_ViewProjection", viewProj);

		StartBatch();
	}

	void Renderer2D::BeginScene(const OrthographicCamera& camera)
	{
		ATN_PROFILE_FUNCTION();

		s_Data.QuadShader->Bind();
		s_Data.QuadShader->SetMat4("u_ViewProjection", camera.GetViewProjectionMatrix());

		s_Data.CircleShader->Bind();
		s_Data.CircleShader->SetMat4("u_ViewProjection", camera.GetViewProjectionMatrix());

		s_Data.LineShader->Bind();
		s_Data.LineShader->SetMat4("u_ViewProjection", camera.GetViewProjectionMatrix());

		StartBatch();
	}

	void Renderer2D::BeginScene(const EditorCamera& camera)
	{
		ATN_PROFILE_FUNCTION();

		s_Data.QuadShader->Bind();
		s_Data.QuadShader->SetMat4("u_ViewProjection", camera.GetViewProjection());

		s_Data.CircleShader->Bind();
		s_Data.CircleShader->SetMat4("u_ViewProjection", camera.GetViewProjection());

		s_Data.LineShader->Bind();
		s_Data.LineShader->SetMat4("u_ViewProjection", camera.GetViewProjection());

		StartBatch();
	}

	void Renderer2D::EndScene()
	{
		ATN_PROFILE_FUNCTION();

		Flush();
	}

	void Renderer2D::Flush()
	{
		ATN_PROFILE_FUNCTION();

		if (s_Data.QuadIndexCount)
		{
			uint64 dataSize = (byte*)s_Data.QuadVertexBufferPointer - (byte*)s_Data.QuadVertexBufferBase;
			s_Data.QuadVertexBuffer->SetData(s_Data.QuadVertexBufferBase, (uint32)dataSize);

			for (uint32 i = 0; i < s_Data.TextureSlotIndex; ++i)
				s_Data.TextureSlots[i]->Bind(i);

			s_Data.QuadShader->Bind();
			RenderCommand::DrawTriangles(s_Data.QuadVertexArray, s_Data.QuadIndexCount);
			s_Data.Stats.DrawCalls++;
		}

		if (s_Data.CircleIndexCount)
		{
			uint64 dataSize = (byte*)s_Data.CircleVertexBufferPointer - (byte*)s_Data.CircleVertexBufferBase;
			s_Data.CircleVertexBuffer->SetData(s_Data.CircleVertexBufferBase, (uint32)dataSize);

			s_Data.CircleShader->Bind();
			RenderCommand::DrawTriangles(s_Data.CircleVertexArray, s_Data.CircleIndexCount);
			s_Data.Stats.DrawCalls++;
		}

		if (s_Data.LineVertexCount)
		{
			uint64 dataSize = (byte*)s_Data.LineVertexBufferPointer - (byte*)s_Data.LineVertexBufferBase;
			s_Data.LineVertexBuffer->SetData(s_Data.LineVertexBufferBase, (uint32)dataSize);

			s_Data.LineShader->Bind();
			RenderCommand::SetLineWidth(s_Data.LineWidth);
			RenderCommand::DrawLines(s_Data.LineVertexArray, s_Data.LineVertexCount);
			s_Data.Stats.DrawCalls++;
		}
	}

	void Renderer2D::StartBatch()
	{
		s_Data.QuadIndexCount = 0;
		s_Data.QuadVertexBufferPointer = s_Data.QuadVertexBufferBase;
		s_Data.TextureSlotIndex = 1;

		s_Data.CircleIndexCount = 0;
		s_Data.CircleVertexBufferPointer = s_Data.CircleVertexBufferBase;

		s_Data.LineVertexCount = 0;
		s_Data.LineVertexBufferPointer = s_Data.LineVertexBufferBase;
	}

	void Renderer2D::NextBatch()
	{
		Flush();
		StartBatch();
	}

	void Renderer2D::DrawQuad(const Vector2& position, const Vector2& size, const LinearColor& color)
	{
		DrawQuad({ position.x, position.y, 0.f }, size, color);
	}

	void Renderer2D::DrawQuad(const Vector3& position, const Vector2& size, const LinearColor& color)
	{
		Matrix4 transform = ScaleMatrix(Vector3(size.x, size.y, 1.f)).Translate(position);

		DrawQuad(transform, color);
	}

	void Renderer2D::DrawQuad(const Vector2& position, const Vector2& size, const Texture2DInstance& texture, const LinearColor& tint, float tilingFactor)
	{
		DrawQuad({ position.x, position.y, 0.f }, size, texture, tint, tilingFactor);
	}

	void Renderer2D::DrawQuad(const Vector3& position, const Vector2& size, const Texture2DInstance& texture, const LinearColor& tint, float tilingFactor)
	{
		Matrix4 transform = ScaleMatrix(Vector3(size.x, size.y, 1.f)).Translate(position);

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
			s_Data.QuadVertexBufferPointer->Position = s_Data.QuadVertexPositions[i] * transform;
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
			s_Data.QuadVertexBufferPointer->Position = s_Data.QuadVertexPositions[i] * transform;
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


	void Renderer2D::DrawCircle(const Matrix4& transform, const LinearColor& color, float thickness, float fade, int entityID)
	{
		ATN_PROFILE_FUNCTION();

		if (s_Data.CircleIndexCount >= Renderer2DData::MaxIndices)
			NextBatch();

		for (SIZE_T i = 0; i < 4; ++i)
		{
			s_Data.CircleVertexBufferPointer->WorldPosition = s_Data.QuadVertexPositions[i] * transform;
			s_Data.CircleVertexBufferPointer->LocalPosition = s_Data.QuadVertexPositions[i] * 2.f;
			s_Data.CircleVertexBufferPointer->Color = color;
			s_Data.CircleVertexBufferPointer->Thickness = thickness;
			s_Data.CircleVertexBufferPointer->Fade = fade;
			s_Data.CircleVertexBufferPointer->EntityID = entityID;
			s_Data.CircleVertexBufferPointer++;
		}

		s_Data.CircleIndexCount += 6;

		s_Data.Stats.QuadCount++;
	}

	void Renderer2D::DrawLine(const Vector3& p0, const Vector3& p1, const LinearColor& color, int entityID) 
	{
		ATN_PROFILE_FUNCTION();

		if (s_Data.LineVertexCount >= Renderer2DData::MaxIndices)
			NextBatch();

		s_Data.LineVertexBufferPointer->Position = p0;
		s_Data.LineVertexBufferPointer->Color = color;
		s_Data.LineVertexBufferPointer->EntityID = entityID;
		s_Data.LineVertexBufferPointer++;
		
		s_Data.LineVertexBufferPointer->Position = p1;
		s_Data.LineVertexBufferPointer->Color = color;
		s_Data.LineVertexBufferPointer->EntityID = entityID;
		s_Data.LineVertexBufferPointer++;

		s_Data.LineVertexCount += 2;
	}

	float Renderer2D::GetLineWidth()
	{
		return s_Data.LineWidth;
	}

	void Renderer2D::SetLineWidth(float width)
	{
		s_Data.LineWidth = width;
	}

	void Renderer2D::DrawRect(const Vector3& position, const Vector2& size, const LinearColor& color, int entityID)
	{
		Vector3 p0 = Vector3(position.x - size.x * 0.5f, position.y - size.y * 0.5f, position.z);
		Vector3 p1 = Vector3(position.x + size.x * 0.5f, position.y - size.y * 0.5f, position.z);
		Vector3 p2 = Vector3(position.x + size.x * 0.5f, position.y + size.y * 0.5f, position.z);
		Vector3 p3 = Vector3(position.x - size.x * 0.5f, position.y + size.y * 0.5f, position.z);

		DrawLine(p0, p1, color, entityID);
		DrawLine(p1, p2, color, entityID);
		DrawLine(p2, p3, color, entityID);
		DrawLine(p3, p0, color, entityID);
	}

	void Renderer2D::DrawRect(const Matrix4& transform, const LinearColor& color, int entityID)
	{
		Vector3 lineVertices[4];
		for (SIZE_T i = 0; i < 4; ++i)
			lineVertices[i] = s_Data.QuadVertexPositions[i] * transform;

		DrawLine(lineVertices[0], lineVertices[1], color, entityID);
		DrawLine(lineVertices[1], lineVertices[2], color, entityID);
		DrawLine(lineVertices[2], lineVertices[3], color, entityID);
		DrawLine(lineVertices[3], lineVertices[0], color, entityID);
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
