#include "SceneRenderer2D.h"

#include "Athena/Math/Transforms.h"

#include "Athena/Renderer/GPUBuffers.h"
#include "Athena/Renderer/Renderer.h"
#include "Athena/Renderer/SceneRenderer.h"
#include "Athena/Renderer/Shader.h"


namespace Athena
{
	struct QuadVertex
	{
		Vector3 Position;
		int EntityID = 0;
		LinearColor Color;
		Vector2 TexCoord;
		float TexIndex;
		float TilingFactor;
	};

	struct CircleVertex
	{
		Vector3 WorldPosition;
		int EntityID = 0;
		Vector3 LocalPosition;
		LinearColor Color;
		float Thickness;
		float Fade;
	};

	struct LineVertex
	{
		Vector3 Position;
		int EntityID = 0;
		LinearColor Color;
	};

	struct SceneRenderer2DData
	{
		// Max Geometry per batch
		static const uint32 MaxQuads = 500;
		static const uint32 MaxQuadVertices = MaxQuads * 4;
		static const uint32 MaxCircles = 300;
		static const uint32 MaxCircleVertices = MaxCircles * 4;
		static const uint32 MaxLines = 300;
		static const uint32 MaxLineVertices = MaxLines * 2;
		static const uint32 MaxIndices = Math::Max(MaxQuads, MaxCircles, MaxLines) * 6; // shared indices

		static const uint32 MaxTextureSlots = 32;   // TODO: RenderCaps

		Ref<VertexBuffer> QuadVertexBuffer;
		Ref<VertexBuffer> CircleVertexBuffer;
		Ref<VertexBuffer> LineVertexBuffer;

		uint32 QuadIndexCount = 0;
		QuadVertex* QuadVertexBufferBase = nullptr;
		QuadVertex* QuadVertexBufferPointer = nullptr;

		uint32 CircleIndexCount = 0;
		CircleVertex* CircleVertexBufferBase = nullptr;
		CircleVertex* CircleVertexBufferPointer = nullptr;

		uint32 LineVertexCount = 0;
		LineVertex* LineVertexBufferBase = nullptr;
		LineVertex* LineVertexBufferPointer = nullptr;

		std::array<Ref<Texture2D>, MaxTextureSlots> TextureSlots;
		uint32 TextureSlotIndex = 1; // 0 - white texture

		Vector4 QuadVertexPositions[4];

		struct CameraData
		{
			Matrix4 ViewProjection;
		};
		CameraData CameraBuffer;
		Ref<ConstantBuffer> CameraConstantBuffer;

		bool DrawEntityID = false;
	};

	static SceneRenderer2DData s_Data;

	void SceneRenderer2D::Init()
	{
		BufferLayout layout =
		{
			{ ShaderDataType::Float3, "a_Position"     },
			{ ShaderDataType::Int,    "a_EntityID"     },
			{ ShaderDataType::Float4, "a_Color"        },
			{ ShaderDataType::Float2, "a_TexCoord"     },
			{ ShaderDataType::Float,  "a_TexIndex"     },
			{ ShaderDataType::Float,  "a_TilingFactor" },
		};

		s_Data.QuadVertexBufferBase = new QuadVertex[SceneRenderer2DData::MaxQuadVertices];

		uint32* indices = new uint32[SceneRenderer2DData::MaxIndices];
		uint32 offset = 0;
		for (uint32 i = 0; i < SceneRenderer2DData::MaxIndices; i += 6)
		{
			indices[i + 0] = offset + 0;
			indices[i + 1] = offset + 1;
			indices[i + 2] = offset + 2;

			indices[i + 3] = offset + 2;
			indices[i + 4] = offset + 3;
			indices[i + 5] = offset + 0;

			offset += 4;
		}

		Ref<IndexBuffer> indexBuffer = IndexBuffer::Create(indices, SceneRenderer2DData::MaxIndices);
		delete[] indices;

		VertexBufferCreateInfo vertexBufferInfo;
		vertexBufferInfo.Data = nullptr;
		vertexBufferInfo.Size = SceneRenderer2DData::MaxQuadVertices * sizeof(QuadVertex);
		vertexBufferInfo.Layout = layout;
		vertexBufferInfo.IndexBuffer = indexBuffer;
		vertexBufferInfo.Usage = BufferUsage::DYNAMIC;

		s_Data.QuadVertexBuffer = VertexBuffer::Create(vertexBufferInfo);

		layout =
		{
			{ ShaderDataType::Float3, "a_WorldPosition"  },
			{ ShaderDataType::Int,    "a_EntityID"       },
			{ ShaderDataType::Float3, "a_LocalPosition"  },
			{ ShaderDataType::Float4, "a_Color"			 },
			{ ShaderDataType::Float,  "a_Thickness"		 },
			{ ShaderDataType::Float,  "a_Fade"			 },
		};

		vertexBufferInfo.Data = nullptr;
		vertexBufferInfo.Size = SceneRenderer2DData::MaxCircles * sizeof(CircleVertex);
		vertexBufferInfo.Layout = layout;
		vertexBufferInfo.IndexBuffer = indexBuffer;
		vertexBufferInfo.Usage = BufferUsage::DYNAMIC;

		s_Data.CircleVertexBuffer = VertexBuffer::Create(vertexBufferInfo);

		s_Data.CircleVertexBufferBase = new CircleVertex[SceneRenderer2DData::MaxCircleVertices];

		layout =
		{
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Int,    "a_EntityID" },
			{ ShaderDataType::Float4, "a_Color"    },
		};

		vertexBufferInfo.Data = nullptr;
		vertexBufferInfo.Size = SceneRenderer2DData::MaxLines * sizeof(LineVertex);
		vertexBufferInfo.Layout = layout;
		vertexBufferInfo.IndexBuffer = indexBuffer;
		vertexBufferInfo.Usage = BufferUsage::DYNAMIC;

		s_Data.LineVertexBuffer = VertexBuffer::Create(vertexBufferInfo);

		s_Data.LineVertexBufferBase = new LineVertex[SceneRenderer2DData::MaxLineVertices];


		s_Data.TextureSlots[0] = Renderer::GetWhiteTexture();

		s_Data.QuadVertexPositions[0] = { -0.5f, -0.5f, 0.f, 1.f };
		s_Data.QuadVertexPositions[1] = { 0.5f, -0.5f, 0.f, 1.f };
		s_Data.QuadVertexPositions[2] = { 0.5f, 0.5f, 0.f, 1.f };
		s_Data.QuadVertexPositions[3] = { -0.5f, 0.5f, 0.f, 1.f };

		s_Data.CameraConstantBuffer = ConstantBuffer::Create(sizeof(SceneRenderer2DData::CameraData), BufferBinder::RENDERER2D_CAMERA_DATA);
	}

	void SceneRenderer2D::Shutdown()
	{
		delete[] s_Data.QuadVertexBufferBase;
		delete[] s_Data.CircleVertexBufferBase;
		delete[] s_Data.LineVertexBufferBase;
	}

	void SceneRenderer2D::EntityIDEnable(bool enable)
	{
		s_Data.DrawEntityID = enable;
	}

	void SceneRenderer2D::BeginScene(const Matrix4& viewMatrix, const Matrix4& projectionMatrix)
	{
		s_Data.CameraBuffer.ViewProjection = viewMatrix * projectionMatrix;
		s_Data.CameraConstantBuffer->SetData(&s_Data.CameraBuffer, sizeof(SceneRenderer2DData::CameraData));

		if (!s_Data.DrawEntityID)
		{
			//RenderPass renderPass;
			//renderPass.TargetFramebuffer = SceneRenderer::GetFinalFramebuffer();
			//renderPass.ClearBit = CLEAR_NONE_BIT;
			//renderPass.Name = "2D Pass";

			//Renderer::BeginRenderPass(renderPass);
		}

		StartBatch();
	}

	void SceneRenderer2D::EndScene()
	{
		if (s_Data.DrawEntityID)	// TODO: Remove
		{
			FlushEntityIDs();
		}
		else
		{
			Flush();
			//Renderer::EndRenderPass();
		}
	}

	void SceneRenderer2D::Flush()
	{
		//if (s_Data.DrawEntityID)
		//	Renderer::BindShader("Renderer2D_EntityID");

		//if (s_Data.QuadIndexCount)
		//{
		//	uint64 dataSize = (byte*)s_Data.QuadVertexBufferPointer - (byte*)s_Data.QuadVertexBufferBase;
		//	s_Data.QuadVertexBuffer->SetData(s_Data.QuadVertexBufferBase, (uint32)dataSize);

		//	for (uint32 i = 0; i < s_Data.TextureSlotIndex; ++i)
		//		s_Data.TextureSlots[i]->Bind(i);

		//	Renderer::BindShader("Renderer2D_Quad");
		//	//Renderer::DrawTriangles(s_Data.QuadVertexBuffer, s_Data.QuadIndexCount);
		//}

		//if (s_Data.CircleIndexCount)
		//{
		//	uint64 dataSize = (byte*)s_Data.CircleVertexBufferPointer - (byte*)s_Data.CircleVertexBufferBase;
		//	s_Data.CircleVertexBuffer->SetData(s_Data.CircleVertexBufferBase, (uint32)dataSize);

		//	Renderer::BindShader("Renderer2D_Circle");
		//	//Renderer::DrawTriangles(s_Data.CircleVertexBuffer, s_Data.CircleIndexCount);
		//}

		//if (s_Data.LineVertexCount)
		//{
		//	uint64 dataSize = (byte*)s_Data.LineVertexBufferPointer - (byte*)s_Data.LineVertexBufferBase;
		//	s_Data.LineVertexBuffer->SetData(s_Data.LineVertexBufferBase, (uint32)dataSize);

		//	Renderer::BindShader("Renderer2D_Line");
		//	//Renderer::DrawLines(s_Data.LineVertexBuffer, s_Data.LineVertexCount);
		//}
	}

	void SceneRenderer2D::FlushEntityIDs()
	{
		//Renderer::BindShader("Renderer2D_EntityID");

		//if (s_Data.QuadIndexCount)
		//{
		//	uint64 dataSize = (byte*)s_Data.QuadVertexBufferPointer - (byte*)s_Data.QuadVertexBufferBase;
		//	s_Data.QuadVertexBuffer->SetData(s_Data.QuadVertexBufferBase, (uint32)dataSize);

		//	//Renderer::DrawTriangles(s_Data.QuadVertexBuffer, s_Data.QuadIndexCount);
		//}

		//if (s_Data.CircleIndexCount)
		//{
		//	uint64 dataSize = (byte*)s_Data.CircleVertexBufferPointer - (byte*)s_Data.CircleVertexBufferBase;
		//	s_Data.CircleVertexBuffer->SetData(s_Data.CircleVertexBufferBase, (uint32)dataSize);

		//	//Renderer::DrawTriangles(s_Data.CircleVertexBuffer, s_Data.CircleIndexCount);
		//}

		//if (s_Data.LineVertexCount)
		//{
		//	uint64 dataSize = (byte*)s_Data.LineVertexBufferPointer - (byte*)s_Data.LineVertexBufferBase;
		//	s_Data.LineVertexBuffer->SetData(s_Data.LineVertexBufferBase, (uint32)dataSize);

		//	//Renderer::DrawLines(s_Data.LineVertexBuffer, s_Data.LineVertexCount);
		//}
	}

	void SceneRenderer2D::StartBatch()
	{
		s_Data.QuadIndexCount = 0;
		s_Data.QuadVertexBufferPointer = s_Data.QuadVertexBufferBase;
		s_Data.TextureSlotIndex = 1;

		s_Data.CircleIndexCount = 0;
		s_Data.CircleVertexBufferPointer = s_Data.CircleVertexBufferBase;

		s_Data.LineVertexCount = 0;
		s_Data.LineVertexBufferPointer = s_Data.LineVertexBufferBase;
	}

	void SceneRenderer2D::NextBatch()
	{
		if (s_Data.DrawEntityID)
			FlushEntityIDs();
		else
			Flush();

		StartBatch();
	}

	void SceneRenderer2D::DrawQuad(const Vector2& position, const Vector2& size, const LinearColor& color)
	{
		DrawQuad({ position.x, position.y, 0.f }, size, color);
	}

	void SceneRenderer2D::DrawQuad(const Vector3& position, const Vector2& size, const LinearColor& color)
	{
		Matrix4 transform = Math::ScaleMatrix(Vector3(size.x, size.y, 1.f)).Translate(position);

		DrawQuad(transform, color);
	}

	void SceneRenderer2D::DrawQuad(const Vector2& position, const Vector2& size, const Texture2DInstance& texture, const LinearColor& tint, float tilingFactor)
	{
		DrawQuad({ position.x, position.y, 0.f }, size, texture, tint, tilingFactor);
	}

	void SceneRenderer2D::DrawQuad(const Vector3& position, const Vector2& size, const Texture2DInstance& texture, const LinearColor& tint, float tilingFactor)
	{
		Matrix4 transform = ScaleMatrix(Vector3(size.x, size.y, 1.f)).Translate(position);

		DrawQuad(transform, texture, tint, tilingFactor);
	}

	void SceneRenderer2D::DrawRotatedQuad(const Vector2& position, const Vector2& size, float rotation, const LinearColor& color)
	{
		DrawRotatedQuad({ position.x, position.y, 0.f }, size, rotation, color);
	}

	void SceneRenderer2D::DrawRotatedQuad(const Vector3& position, const Vector2& size, float rotation, const LinearColor& color)
	{
		Matrix4 transform =
			Math::ScaleMatrix(Vector3(size.x, size.y, 1.f)).Rotate(rotation, Vector3(0.f, 0.f, 1.f)).Translate(position);

		DrawQuad(transform, color);
	}

	void SceneRenderer2D::DrawRotatedQuad(const Vector2& position, const Vector2& size, float rotation, const Texture2DInstance& texture, const LinearColor& tint, float tilingFactor)
	{
		DrawRotatedQuad({ position.x, position.y, 0.f }, size, rotation, texture, tint, tilingFactor);
	}

	void SceneRenderer2D::DrawRotatedQuad(const Vector3& position, const Vector2& size, float rotation, const Texture2DInstance& texture, const LinearColor& tint, float tilingFactor)
	{
		Matrix4 transform =
			Math::ScaleMatrix(Vector3(size.x, size.y, 1.f)).Rotate(rotation, Vector3(0.f, 0.f, 1.f)).Translate(position);

		DrawQuad(transform, texture, tint, tilingFactor);
	}

	void SceneRenderer2D::DrawQuad(const Matrix4& transform, const LinearColor& color, int32 entityID)
	{
		if (s_Data.QuadIndexCount >= SceneRenderer2DData::MaxIndices)
			NextBatch();

		constexpr uint32 QuadVertexCount = 4;
		constexpr float textureIndex = 0.f; // White Texture
		constexpr Vector2 textureCoords[4] = { {0.f, 0.f}, {1.f, 0.f}, {1.f, 1.f}, {0.f, 1.f} };
		constexpr float tilingFactor = 1.f;

		for (uint32 i = 0; i < QuadVertexCount; ++i)
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
	}

	void SceneRenderer2D::DrawQuad(const Matrix4& transform, const Texture2DInstance& texture, const LinearColor& tint, float tilingFactor, int32 entityID)
	{
		//if (s_Data.QuadIndexCount >= SceneRenderer2DData::MaxIndices)
		//	NextBatch();

		//constexpr uint32 QuadVertexCount = 4;
		//const auto& texCoords = texture.GetTexCoords();
		//float textureIndex = 0.0f;

		//for (uint32 i = 1; i < s_Data.TextureSlotIndex; ++i)
		//{
		//	if (*s_Data.TextureSlots[i] == *(texture.GetNativeTexture()))
		//	{
		//		textureIndex = (float)i;
		//		break;
		//	}
		//}

		//if (textureIndex == 0.0f)
		//{
		//	if (s_Data.TextureSlotIndex >= SceneRenderer2DData::MaxTextureSlots)
		//		NextBatch();

		//	textureIndex = (float)s_Data.TextureSlotIndex;
		//	s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture.GetNativeTexture();
		//	s_Data.TextureSlotIndex++;
		//}

		//for (uint32 i = 0; i < QuadVertexCount; ++i)
		//{
		//	s_Data.QuadVertexBufferPointer->Position = s_Data.QuadVertexPositions[i] * transform;
		//	s_Data.QuadVertexBufferPointer->Color = tint;
		//	s_Data.QuadVertexBufferPointer->TexCoord = texCoords[i];
		//	s_Data.QuadVertexBufferPointer->TexIndex = textureIndex;
		//	s_Data.QuadVertexBufferPointer->TilingFactor = tilingFactor;
		//	s_Data.QuadVertexBufferPointer->EntityID = entityID;
		//	s_Data.QuadVertexBufferPointer++;
		//}

		//s_Data.QuadIndexCount += 6;
	}


	void SceneRenderer2D::DrawCircle(const Matrix4& transform, const LinearColor& color, float thickness, float fade, int32 entityID)
	{
		if (s_Data.CircleIndexCount >= SceneRenderer2DData::MaxIndices)
			NextBatch();

		for (uint32 i = 0; i < 4; ++i)
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
	}

	void SceneRenderer2D::DrawLine(const Vector3& p0, const Vector3& p1, const LinearColor& color, float width, int32 entityID)
	{
		if (width > 0.f && width <= 1.f)
		{
			if (s_Data.LineVertexCount >= SceneRenderer2DData::MaxIndices)
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
		else if(width > 1.f)
		{
			Vector3 dir = p1 - p0;
			Vector3 normal = Vector3(-dir.y, dir.x, 0.f).Normalize() * width / 1000.f;

			Vector3 positions[4] = { p1 - normal, p1 + normal, p0 + normal, p0 - normal };

			if (s_Data.QuadIndexCount >= SceneRenderer2DData::MaxIndices)
				NextBatch();

			constexpr uint32 QuadVertexCount = 4;
			constexpr float textureIndex = 0.f; // White Texture
			constexpr Vector2 textureCoords[4] = { {0.f, 0.f}, {1.f, 0.f}, {1.f, 1.f}, {0.f, 1.f} };
			constexpr float tilingFactor = 1.f;

			for (uint32 i = 0; i < QuadVertexCount; ++i)
			{
				s_Data.QuadVertexBufferPointer->Position = positions[i];
				s_Data.QuadVertexBufferPointer->Color = color;
				s_Data.QuadVertexBufferPointer->TexCoord = textureCoords[i];
				s_Data.QuadVertexBufferPointer->TexIndex = textureIndex;
				s_Data.QuadVertexBufferPointer->TilingFactor = tilingFactor;
				s_Data.QuadVertexBufferPointer->EntityID = entityID;
				s_Data.QuadVertexBufferPointer++;
			}

			s_Data.QuadIndexCount += 6;
		}
	}

	void SceneRenderer2D::DrawRect(const Vector3& position, const Vector2& size, const LinearColor& color, float lineWidth, int32 entityID)
	{
		Vector3 p0 = Vector3(position.x - size.x * 0.5f, position.y - size.y * 0.5f, position.z);
		Vector3 p1 = Vector3(position.x + size.x * 0.5f, position.y - size.y * 0.5f, position.z);
		Vector3 p2 = Vector3(position.x + size.x * 0.5f, position.y + size.y * 0.5f, position.z);
		Vector3 p3 = Vector3(position.x - size.x * 0.5f, position.y + size.y * 0.5f, position.z);

		DrawLine(p0, p1, color, lineWidth, entityID);
		DrawLine(p1, p2, color, lineWidth, entityID);
		DrawLine(p2, p3, color, lineWidth, entityID);
		DrawLine(p3, p0, color, lineWidth, entityID);
	}

	void SceneRenderer2D::DrawRect(const Matrix4& transform, const LinearColor& color, float lineWidth, int32 entityID)
	{
		Vector3 lineVertices[4];
		for (uint32 i = 0; i < 4; ++i)
			lineVertices[i] = s_Data.QuadVertexPositions[i] * transform;

		DrawLine(lineVertices[0], lineVertices[1], color, lineWidth, entityID);
		DrawLine(lineVertices[1], lineVertices[2], color, lineWidth, entityID);
		DrawLine(lineVertices[2], lineVertices[3], color, lineWidth, entityID);
		DrawLine(lineVertices[3], lineVertices[0], color, lineWidth, entityID);
	}
}
