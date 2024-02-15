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

		m_IndexBuffer = IndexBuffer::Create(indexBufferInfo);
		delete[] indices;

		m_QuadVertexBufferBase = new QuadVertex[s_MaxQuadVertices];
		m_CircleVertexBufferBase = new CircleVertex[s_MaxCircleVertices];
		m_LineVertexBufferBase = new LineVertex[s_MaxLineVertices];

		m_QuadVertexBuffers.resize(Renderer::GetFramesInFlight());
		m_CircleVertexBuffers.resize(Renderer::GetFramesInFlight());
		m_LineVertexBuffers.resize(Renderer::GetFramesInFlight());

		m_TextureSlots[0] = Renderer::GetWhiteTexture();

		m_QuadVertexPositions[0] = { -0.5f,0.5f, 0.f, 1.f };
		m_QuadVertexPositions[1] = { 0.5f, 0.5f, 0.f, 1.f };
		m_QuadVertexPositions[2] = { 0.5f, -0.5f, 0.f, 1.f };
		m_QuadVertexPositions[3] = { -0.5f, -0.5f, 0.f, 1.f };

		
		PipelineCreateInfo pipelineInfo;
		pipelineInfo.Name = "QuadPipeline";
		pipelineInfo.RenderPass = renderPass;
		pipelineInfo.Shader = Renderer::GetShaderPack()->Get("Renderer2D_Quad");
		pipelineInfo.Topology = Topology::TRIANGLE_LIST;
		pipelineInfo.CullMode = CullMode::NONE;
		pipelineInfo.DepthCompare = DepthCompare::LESS_OR_EQUAL;
		pipelineInfo.BlendEnable = true;

		m_QuadPipeline = Pipeline::Create(pipelineInfo);
		m_QuadPipeline->Bake();

		m_QuadMaterials.push_back(Material::Create(pipelineInfo.Shader, pipelineInfo.Name));

		pipelineInfo.Name = "CirclePipeline";
		pipelineInfo.RenderPass = renderPass;
		pipelineInfo.Shader = Renderer::GetShaderPack()->Get("Renderer2D_Circle");
		pipelineInfo.Topology = Topology::TRIANGLE_LIST;
		pipelineInfo.CullMode = CullMode::NONE;
		pipelineInfo.DepthCompare = DepthCompare::LESS_OR_EQUAL;
		pipelineInfo.BlendEnable = true;

		m_CirclePipeline = Pipeline::Create(pipelineInfo);
		m_CirclePipeline->Bake();

		m_CircleMaterial = Material::Create(pipelineInfo.Shader, pipelineInfo.Name);

		pipelineInfo.Name = "LinePipeline";
		pipelineInfo.RenderPass = renderPass;
		pipelineInfo.Shader = Renderer::GetShaderPack()->Get("Renderer2D_Line");
		pipelineInfo.Topology = Topology::LINE_LIST;
		pipelineInfo.CullMode = CullMode::NONE;
		pipelineInfo.DepthCompare = DepthCompare::LESS_OR_EQUAL;
		pipelineInfo.BlendEnable = true;
		pipelineInfo.LineWidth = 1;

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

		Matrix4 viewProj = viewMatrix * projectionMatrix;

		for(const auto& quadMaterial : m_QuadMaterials)
			quadMaterial->Set("u_ViewProjection", viewProj);

		m_CircleMaterial->Set("u_ViewProjection", viewProj);
		m_LineMaterial->Set("u_ViewProjection", viewProj);
		
		m_QuadIndexCount = 0;
		m_QuadVertexBufferPointer = m_QuadVertexBufferBase;
		m_TextureSlotIndex = 1;
		m_QuadVertexBufferIndex = 0;

		m_CircleIndexCount = 0;
		m_CircleVertexBufferPointer = m_CircleVertexBufferBase;
		m_CircleVertexBufferIndex = 0;

		m_LineVertexCount = 0;
		m_LineVertexBufferPointer = m_LineVertexBufferBase;
		m_LineVertexBufferIndex = 0;
	}

	void SceneRenderer2D::EndScene()
	{
		FlushQuads();
		FlushCircles();
		FlushLines();

		auto commandBuffer = m_RenderCommandBuffer;

		if (!m_QuadDrawList.empty())
		{
			m_QuadPipeline->Bind(commandBuffer);

			Ref<Material> prevMaterial = m_QuadMaterials[m_QuadDrawList[0].VertexBufferIndex];
			prevMaterial->Bind(commandBuffer);

			for (const auto& drawCall : m_QuadDrawList)
			{
				Ref<Material> material = m_QuadMaterials[drawCall.VertexBufferIndex];
				Ref<VertexBuffer> buffer = m_QuadVertexBuffers[Renderer::GetCurrentFrameIndex()][drawCall.VertexBufferIndex];

				if (prevMaterial != material)
				{
					prevMaterial = material;
					prevMaterial->Bind(commandBuffer);
				}

				Renderer::RenderGeometry(commandBuffer, m_QuadPipeline, buffer, material, drawCall.VertexCount);
			}
		}

		if (!m_CircleDrawList.empty())
		{
			m_CirclePipeline->Bind(commandBuffer);
			for (const auto& drawCall : m_CircleDrawList)
			{
				Ref<VertexBuffer> buffer = m_CircleVertexBuffers[Renderer::GetCurrentFrameIndex()][drawCall.VertexBufferIndex];
				Renderer::RenderGeometry(commandBuffer, m_CirclePipeline, buffer, m_CircleMaterial, drawCall.VertexCount);
			}
		}

		if (!m_LineDrawList.empty())
		{
			m_LinePipeline->Bind(commandBuffer);
			for (const auto& drawCall : m_LineDrawList)
			{
				Ref<VertexBuffer> buffer = m_LineVertexBuffers[Renderer::GetCurrentFrameIndex()][drawCall.VertexBufferIndex];
				Renderer::RenderGeometry(commandBuffer, m_LinePipeline, buffer, m_LineMaterial, drawCall.VertexCount);
			}
		}

		m_QuadDrawList.clear();
		m_CircleDrawList.clear();
		m_LineDrawList.clear();
	}

	void SceneRenderer2D::FlushQuads()
	{
		if (m_QuadIndexCount == 0)
			return;

		uint32 frameIndex = Renderer::GetCurrentFrameIndex();
		if (m_QuadVertexBufferIndex == m_QuadVertexBuffers[frameIndex].size())
		{
			VertexBufferCreateInfo vertexBufferInfo;
			vertexBufferInfo.Name = std::format("Renderer2D_QuadVB_f{}_{}", frameIndex, m_QuadVertexBuffers[frameIndex].size());
			vertexBufferInfo.Data = nullptr;
			vertexBufferInfo.Size = s_MaxQuads * sizeof(QuadVertex);
			vertexBufferInfo.IndexBuffer = m_IndexBuffer;
			vertexBufferInfo.Usage = BufferUsage::DYNAMIC;

			Ref<VertexBuffer> newBuffer = VertexBuffer::Create(vertexBufferInfo);
			m_QuadVertexBuffers[frameIndex].push_back(newBuffer);
		}

		Ref<VertexBuffer> buffer = m_QuadVertexBuffers[Renderer::GetCurrentFrameIndex()][m_QuadVertexBufferIndex];
		uint64 dataSize = (byte*)m_QuadVertexBufferPointer - (byte*)m_QuadVertexBufferBase;

		buffer->SetData(m_QuadVertexBufferBase, dataSize);

		for (uint32 i = 0; i < m_TextureSlotIndex; ++i)
			m_QuadMaterials[m_QuadVertexBufferIndex]->Set("u_Textures", m_TextureSlots[i], i);

		DrawCall2D drawCall;
		drawCall.VertexBufferIndex = m_QuadVertexBufferIndex;
		drawCall.VertexCount = m_QuadIndexCount;

		m_QuadDrawList.push_back(drawCall);

		m_QuadIndexCount = 0;
		m_QuadVertexBufferPointer = m_QuadVertexBufferBase;
		m_TextureSlotIndex = 1;
		m_QuadVertexBufferIndex++;

		if (m_QuadVertexBufferIndex >= m_QuadMaterials.size())
		{
			Ref<Material> newMaterial = Material::Create(m_QuadMaterials[0]->GetShader(), std::format("Renderer2D_Quad_{}", m_QuadMaterials.size()));
			newMaterial->Set("u_ViewProjection", m_QuadMaterials[0]->Get<Matrix4>("u_ViewProjection"));
			m_QuadMaterials.push_back(newMaterial);
		}
	}

	void SceneRenderer2D::FlushCircles()
	{
		if (m_CircleIndexCount == 0)
			return;

		uint32 frameIndex = Renderer::GetCurrentFrameIndex();
		if (m_CircleVertexBufferIndex == m_CircleVertexBuffers[frameIndex].size())
		{
			VertexBufferCreateInfo vertexBufferInfo;
			vertexBufferInfo.Name = std::format("Renderer2D_CircleVB_f{}_{}", frameIndex, m_CircleVertexBuffers[frameIndex].size());
			vertexBufferInfo.Data = nullptr;
			vertexBufferInfo.Size = s_MaxCircles * sizeof(CircleVertex);
			vertexBufferInfo.IndexBuffer = m_IndexBuffer;
			vertexBufferInfo.Usage = BufferUsage::DYNAMIC;

			Ref<VertexBuffer> newBuffer = VertexBuffer::Create(vertexBufferInfo);
			m_CircleVertexBuffers[frameIndex].push_back(newBuffer);
		}

		Ref<VertexBuffer> buffer = m_CircleVertexBuffers[Renderer::GetCurrentFrameIndex()][m_CircleVertexBufferIndex];
		uint64 dataSize = (byte*)m_CircleVertexBufferPointer - (byte*)m_CircleVertexBufferBase;

		buffer->SetData(m_CircleVertexBufferBase, dataSize);

		DrawCall2D drawCall;
		drawCall.VertexBufferIndex = m_CircleVertexBufferIndex;
		drawCall.VertexCount = m_CircleIndexCount;

		m_CircleDrawList.push_back(drawCall);

		m_CircleIndexCount = 0;
		m_CircleVertexBufferPointer = m_CircleVertexBufferBase;
		m_CircleVertexBufferIndex++;
	}

	void SceneRenderer2D::FlushLines()
	{
		if (m_LineVertexCount == 0)
			return;

		uint32 frameIndex = Renderer::GetCurrentFrameIndex();
		if (m_LineVertexBufferIndex == m_LineVertexBuffers[frameIndex].size())
		{
			VertexBufferCreateInfo vertexBufferInfo;
			vertexBufferInfo.Name = std::format("Renderer2D_LineVB_f{}_{}", frameIndex, m_LineVertexBuffers[frameIndex].size());
			vertexBufferInfo.Data = nullptr;
			vertexBufferInfo.Size = s_MaxLines * sizeof(LineVertex);
			vertexBufferInfo.IndexBuffer = nullptr;
			vertexBufferInfo.Usage = BufferUsage::DYNAMIC;

			Ref<VertexBuffer> newBuffer = VertexBuffer::Create(vertexBufferInfo);
			m_LineVertexBuffers[frameIndex].push_back(newBuffer);
		}

		Ref<VertexBuffer> buffer = m_LineVertexBuffers[Renderer::GetCurrentFrameIndex()][m_LineVertexBufferIndex];
		uint64 dataSize = (byte*)m_LineVertexBufferPointer - (byte*)m_LineVertexBufferBase;

		buffer->SetData(m_LineVertexBufferBase, dataSize);

		DrawCall2D drawCall;
		drawCall.VertexBufferIndex = m_LineVertexBufferIndex;
		drawCall.VertexCount = m_LineVertexCount;

		m_LineDrawList.push_back(drawCall);

		m_LineVertexCount = 0;
		m_LineVertexBufferPointer = m_LineVertexBufferBase;
		m_LineVertexBufferIndex++;
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
			FlushQuads();

		constexpr uint32 QuadVertexCount = 4;
		constexpr float textureIndex = 0.f; // White Texture
		constexpr Vector2 textureCoords[4] = { {0.f, 0.f}, {1.f, 0.f}, {1.f, 1.f}, {0.f, 1.f} };
		constexpr float tilingFactor = 1.f;

		for (uint32 i = 0; i < QuadVertexCount; ++i)
		{
			m_QuadVertexBufferPointer->Position = m_QuadVertexPositions[i] * transform;
			m_QuadVertexBufferPointer->Color = color;
			m_QuadVertexBufferPointer->TexCoords = textureCoords[i] * tilingFactor;
			m_QuadVertexBufferPointer->TexIndex = textureIndex;
			m_QuadVertexBufferPointer++;
		}

		m_QuadIndexCount += 6;
	}

	void SceneRenderer2D::DrawQuad(const Matrix4& transform, const Texture2DInstance& texture, const LinearColor& tint, float tilingFactor)
	{
		if (m_QuadIndexCount >= s_MaxIndices)
			FlushQuads();

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
				FlushQuads();
			
			textureIndex = m_TextureSlotIndex;
			m_TextureSlots[m_TextureSlotIndex] = texture.GetNativeTexture();
			m_TextureSlotIndex++;
		}

		for (uint32 i = 0; i < QuadVertexCount; ++i)
		{
			m_QuadVertexBufferPointer->Position = m_QuadVertexPositions[i] * transform;
			m_QuadVertexBufferPointer->Color = tint;
			m_QuadVertexBufferPointer->TexCoords = texCoords[i] * tilingFactor;
			m_QuadVertexBufferPointer->TexIndex = textureIndex;
			m_QuadVertexBufferPointer++;
		}

		m_QuadIndexCount += 6;
	}


	void SceneRenderer2D::DrawCircle(const Matrix4& transform, const LinearColor& color, float thickness, float fade)
	{
		if (m_CircleIndexCount >= s_MaxIndices)
			FlushCircles();

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
			FlushLines();

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
		m_LinePipeline->SetLineWidth(width);
	}

	float SceneRenderer2D::GetLineWidth()
	{
		return m_LinePipeline->GetLineWidth();
	}
}
