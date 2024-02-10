#include "SceneRenderer2D.h"

#include "Athena/Math/Transforms.h"

#include "Athena/Renderer/Renderer.h"
#include "Athena/Renderer/SceneRenderer.h"
#include "Athena/Renderer/Shader.h"


namespace Athena
{
	Ref<SceneRenderer2D> SceneRenderer2D::Create(const Ref<RenderPass>& renderPass)
	{
		Ref<SceneRenderer2D> result = Ref<SceneRenderer2D>::Create();
		result->Init(renderPass);

		return result;
	}

	SceneRenderer2D::~SceneRenderer2D()
	{
		Shutdown();
	}

	void SceneRenderer2D::Init(const Ref<RenderPass>& renderPass)
	{
		// Shared indices for quads and circles
		uint32* indices = new uint32[s_MaxIndices];
		uint32 offset = 0;
		for (uint32 i = 0; i < s_MaxIndices; i += 6)
		{
			indices[i + 0] = offset + 0;
			indices[i + 1] = offset + 1;
			indices[i + 2] = offset + 2;

			indices[i + 3] = offset + 2;
			indices[i + 4] = offset + 3;
			indices[i + 5] = offset + 0;

			offset += 4;
		}

		IndexBufferCreateInfo indexBufferInfo;
		indexBufferInfo.Name = "Renderer2D_QuadIB";
		indexBufferInfo.Data = indices;
		indexBufferInfo.Count = s_MaxIndices;
		indexBufferInfo.Usage = BufferUsage::STATIC;

		Ref<IndexBuffer> indexBuffer = IndexBuffer::Create(indexBufferInfo);
		delete[] indices;

		// Quad vertex buffer
		m_QuadVertexBufferBase = new QuadVertex[s_MaxQuadVertices];

		VertexBufferCreateInfo vertexBufferInfo;
		vertexBufferInfo.Name = "Renderer2D_QuadVB";
		vertexBufferInfo.Data = nullptr;
		vertexBufferInfo.Size = s_MaxQuadVertices * sizeof(QuadVertex);
		vertexBufferInfo.IndexBuffer = indexBuffer;
		vertexBufferInfo.Usage = BufferUsage::DYNAMIC;

		m_QuadVertexBuffer = VertexBuffer::Create(vertexBufferInfo);

		// Circle vertex buffer
		m_CircleVertexBufferBase = new CircleVertex[s_MaxCircleVertices];

		vertexBufferInfo.Data = nullptr;
		vertexBufferInfo.Size = s_MaxCircles * sizeof(CircleVertex);
		vertexBufferInfo.IndexBuffer = indexBuffer;
		vertexBufferInfo.Usage = BufferUsage::DYNAMIC;

		m_CircleVertexBuffer = VertexBuffer::Create(vertexBufferInfo);

		// Line vertex buffer
		m_LineVertexBufferBase = new LineVertex[s_MaxLineVertices];

		vertexBufferInfo.Data = nullptr;
		vertexBufferInfo.Size = s_MaxLines * sizeof(LineVertex);
		vertexBufferInfo.IndexBuffer = nullptr;
		vertexBufferInfo.Usage = BufferUsage::DYNAMIC;

		m_LineVertexBuffer = VertexBuffer::Create(vertexBufferInfo);

		m_TextureSlots[0] = Renderer::GetWhiteTexture();

		m_QuadVertexPositions[0] = { -0.5f,-0.5f, 0.f, 1.f };
		m_QuadVertexPositions[1] = { 0.5f, -0.5f, 0.f, 1.f };
		m_QuadVertexPositions[2] = { 0.5f, 0.5f, 0.f, 1.f };
		m_QuadVertexPositions[3] = { -0.5f, 0.5f, 0.f, 1.f };

		
		PipelineCreateInfo pipelineInfo;
		pipelineInfo.Name = "QuadPipeline";
		pipelineInfo.RenderPass = renderPass;
		pipelineInfo.Shader = Renderer::GetShaderPack()->Get("Renderer2D_Quad");
		pipelineInfo.Topology = Topology::TRIANGLE_LIST;
		pipelineInfo.CullMode = CullMode::BACK;
		pipelineInfo.DepthCompare = DepthCompare::LESS_OR_EQUAL;
		pipelineInfo.BlendEnable = true;

		m_QuadPipeline = Pipeline::Create(pipelineInfo);
		m_QuadPipeline->SetInput("u_Texture", Renderer::GetWhiteTexture());
		m_QuadPipeline->Bake();

		m_QuadMaterial = Material::Create(pipelineInfo.Shader, pipelineInfo.Name);

		pipelineInfo.Name = "CirclePipeline";
		pipelineInfo.RenderPass = renderPass;
		pipelineInfo.Shader = Renderer::GetShaderPack()->Get("Renderer2D_Circle");
		pipelineInfo.Topology = Topology::LINE_LIST;
		pipelineInfo.CullMode = CullMode::BACK;
		pipelineInfo.DepthCompare = DepthCompare::LESS_OR_EQUAL;
		pipelineInfo.BlendEnable = true;

		m_CirclePipeline = Pipeline::Create(pipelineInfo);
		m_CirclePipeline->Bake();

		m_CircleMaterial = Material::Create(pipelineInfo.Shader, pipelineInfo.Name);

		pipelineInfo.Name = "LinePipeline";
		pipelineInfo.RenderPass = renderPass;
		pipelineInfo.Shader = Renderer::GetShaderPack()->Get("Renderer2D_Line");
		pipelineInfo.Topology = Topology::LINE_LIST;
		pipelineInfo.CullMode = CullMode::BACK;
		pipelineInfo.DepthCompare = DepthCompare::LESS_OR_EQUAL;
		pipelineInfo.BlendEnable = true;
		pipelineInfo.LineWidth = m_LineWidth;

		m_LinePipeline = Pipeline::Create(pipelineInfo);
		m_LinePipeline->Bake();

		m_LineMaterial = Material::Create(pipelineInfo.Shader, pipelineInfo.Name);
	}

	void SceneRenderer2D::Shutdown()
	{
		delete[] m_QuadVertexBufferBase;
		delete[] m_CircleVertexBufferBase;
		delete[] m_LineVertexBufferBase;
	}

	void SceneRenderer2D::OnViewportResize(uint32 width, uint32 height)
	{
		m_QuadPipeline->SetViewport(width, height);
		m_LinePipeline->SetViewport(width, height);
		m_CirclePipeline->SetViewport(width, height);
	}

	void SceneRenderer2D::BeginScene(const Matrix4& viewMatrix, const Matrix4& projectionMatrix)
	{
		m_RenderCommandBuffer = Renderer::GetRenderCommandBuffer();

		m_QuadMaterial->Set("u_ViewProjection", viewMatrix * projectionMatrix);
		m_CircleMaterial->Set("u_ViewProjection", viewMatrix * projectionMatrix);
		m_LineMaterial->Set("u_ViewProjection", viewMatrix * projectionMatrix);
		StartBatch();
	}

	void SceneRenderer2D::EndScene()
	{
		Flush();
	}

	void SceneRenderer2D::Flush()
	{
		//Renderer::Submit([this]()
		//{
			if (m_QuadIndexCount)
			{
				uint64 dataSize = (byte*)m_QuadVertexBufferPointer - (byte*)m_QuadVertexBufferBase;
				m_QuadVertexBuffer->RT_SetData(m_QuadVertexBufferBase, dataSize);
			}

			if (m_CircleIndexCount)
			{
				uint64 dataSize = (byte*)m_CircleVertexBufferPointer - (byte*)m_CircleVertexBufferBase;
				m_CircleVertexBuffer->RT_SetData(m_CircleVertexBufferBase, dataSize);
			}

			if (m_LineVertexCount)
			{
				uint64 dataSize = (byte*)m_LineVertexBufferPointer - (byte*)m_LineVertexBufferBase;
				m_LineVertexBuffer->RT_SetData(m_LineVertexBufferBase, dataSize);
			}
		//});

		auto commandBuffer = m_RenderCommandBuffer;

		if (m_QuadIndexCount)
		{
			//for (uint32 i = 0; i < s_Data.TextureSlotIndex; ++i)
			//	s_Data.TextureSlots[i]->Bind(i);

			m_QuadPipeline->Bind(commandBuffer);
			Renderer::RenderGeometry(commandBuffer, m_QuadPipeline, m_QuadVertexBuffer, m_QuadMaterial, m_QuadIndexCount);
		}

		if (m_CircleIndexCount)
		{
			m_CirclePipeline->Bind(commandBuffer);
			Renderer::RenderGeometry(commandBuffer, m_CirclePipeline, m_CircleVertexBuffer, m_CircleMaterial, m_CircleIndexCount);
		}

		if (m_LineVertexCount)
		{
			m_LinePipeline->Bind(commandBuffer);
			Renderer::RenderGeometry(commandBuffer, m_LinePipeline, m_LineVertexBuffer, m_LineMaterial, m_LineVertexCount);
		}
	}

	void SceneRenderer2D::StartBatch()
	{
		m_QuadIndexCount = 0;
		m_QuadVertexBufferPointer = m_QuadVertexBufferBase;
		m_TextureSlotIndex = 1;

		m_CircleIndexCount = 0;
		m_CircleVertexBufferPointer = m_CircleVertexBufferBase;

		m_LineVertexCount = 0;
		m_LineVertexBufferPointer = m_LineVertexBufferBase;
	}

	void SceneRenderer2D::NextBatch()
	{
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

	void SceneRenderer2D::DrawQuad(const Matrix4& transform, const LinearColor& color)
	{
		if (m_QuadIndexCount >= s_MaxIndices)
			NextBatch();

		constexpr uint32 QuadVertexCount = 4;
		constexpr float textureIndex = 0.f; // White Texture
		constexpr Vector2 textureCoords[4] = { {0.f, 0.f}, {1.f, 0.f}, {1.f, 1.f}, {0.f, 1.f} };
		constexpr float tilingFactor = 1.f;

		for (uint32 i = 0; i < QuadVertexCount; ++i)
		{
			m_QuadVertexBufferPointer->Position = m_QuadVertexPositions[i] * transform;
			m_QuadVertexBufferPointer->Color = color;
			m_QuadVertexBufferPointer->TexCoord = textureCoords[i];
			m_QuadVertexBufferPointer->TexIndex = textureIndex;
			m_QuadVertexBufferPointer->TilingFactor = tilingFactor;
			m_QuadVertexBufferPointer++;
		}

		m_QuadIndexCount += 6;
	}

	void SceneRenderer2D::DrawQuad(const Matrix4& transform, const Texture2DInstance& texture, const LinearColor& tint, float tilingFactor)
	{
		if (m_QuadIndexCount >= s_MaxIndices)
			NextBatch();

		constexpr uint32 QuadVertexCount = 4;
		const auto& texCoords = texture.GetTexCoords();
		int32 textureIndex = 0;

		for (uint32 i = 1; i < m_TextureSlotIndex; ++i)
		{
			if (m_TextureSlots[i]->GetName() == texture.GetNativeTexture()->GetName())
			{
				textureIndex = i;
				break;
			}
		}

		if (textureIndex == 0)
		{
			if (m_TextureSlotIndex >= s_MaxTextureSlots)
				NextBatch();

			textureIndex = m_TextureSlotIndex;
			m_TextureSlots[m_TextureSlotIndex] = texture.GetNativeTexture();
			m_TextureSlotIndex++;
		}

		for (uint32 i = 0; i < QuadVertexCount; ++i)
		{
			m_QuadVertexBufferPointer->Position = m_QuadVertexPositions[i] * transform;
			m_QuadVertexBufferPointer->Color = tint;
			m_QuadVertexBufferPointer->TexCoord = texCoords[i];
			m_QuadVertexBufferPointer->TexIndex = textureIndex;
			m_QuadVertexBufferPointer->TilingFactor = tilingFactor;
			m_QuadVertexBufferPointer++;
		}

		m_QuadIndexCount += 6;
	}


	void SceneRenderer2D::DrawCircle(const Matrix4& transform, const LinearColor& color, float thickness, float fade)
	{
		if (m_CircleIndexCount >= s_MaxIndices)
			NextBatch();

		for (uint32 i = 0; i < 4; ++i)
		{
			m_CircleVertexBufferPointer->WorldPosition = m_QuadVertexPositions[i] * transform;
			m_CircleVertexBufferPointer->LocalPosition = m_QuadVertexPositions[i] * 2.f;
			m_CircleVertexBufferPointer->Color = color;
			m_CircleVertexBufferPointer->Thickness = thickness;
			m_CircleVertexBufferPointer->Fade = fade;
			m_CircleVertexBufferPointer++;
		}

		m_CircleIndexCount += 6;
	}

	void SceneRenderer2D::DrawLine(const Vector3& p0, const Vector3& p1, const LinearColor& color)
	{
		if (m_LineVertexCount >= s_MaxIndices)
			NextBatch();

		m_LineVertexBufferPointer->Position = p0;
		m_LineVertexBufferPointer->Color = color;
		m_LineVertexBufferPointer++;

		m_LineVertexBufferPointer->Position = p1;
		m_LineVertexBufferPointer->Color = color;
		m_LineVertexBufferPointer++;

		m_LineVertexCount += 2;
	}

	void SceneRenderer2D::DrawRect(const Vector3& position, const Vector2& size, const LinearColor& color)
	{
		Vector3 p0 = Vector3(position.x - size.x * 0.5f, position.y - size.y * 0.5f, position.z);
		Vector3 p1 = Vector3(position.x + size.x * 0.5f, position.y - size.y * 0.5f, position.z);
		Vector3 p2 = Vector3(position.x + size.x * 0.5f, position.y + size.y * 0.5f, position.z);
		Vector3 p3 = Vector3(position.x - size.x * 0.5f, position.y + size.y * 0.5f, position.z);

		DrawLine(p0, p1, color);
		DrawLine(p1, p2, color);
		DrawLine(p2, p3, color);
		DrawLine(p3, p0, color);
	}

	void SceneRenderer2D::DrawRect(const Matrix4& transform, const LinearColor& color)
	{
		Vector3 lineVertices[4];
		for (uint32 i = 0; i < 4; ++i)
			lineVertices[i] = m_QuadVertexPositions[i] * transform;

		DrawLine(lineVertices[0], lineVertices[1], color);
		DrawLine(lineVertices[1], lineVertices[2], color);
		DrawLine(lineVertices[2], lineVertices[3], color);
		DrawLine(lineVertices[3], lineVertices[0], color);
	}

	void SceneRenderer2D::SetLineWidth(float width)
	{
		m_LineWidth = width;
	}

	float SceneRenderer2D::GetLineWidth()
	{
		return m_LineWidth;
	}
}
