#include "SceneRenderer2D.h"

#include "Athena/Math/Transforms.h"
#include "Athena/Math/Projections.h"
#include "Athena/Utils/StringUtils.h"
#include "Athena/Renderer/Renderer.h"
#include "Athena/Renderer/SceneRenderer.h"
#include "Athena/Renderer/Shader.h"
#include "Athena/Renderer/TextureGenerator.h"
#include "Athena/Renderer/FontAtlasGeometryData.h"


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
		
		m_IndicesCount.resize(Renderer::GetFramesInFlight());

		IndexBufferCreateInfo indexBufferInfo;
		indexBufferInfo.Name = "Renderer2D_QuadIB";
		indexBufferInfo.Data = nullptr;
		indexBufferInfo.Count = 1 * sizeof(uint32);
		indexBufferInfo.Flags = BufferMemoryFlags::CPU_WRITEABLE;

		m_IndexBuffer = IndexBuffer::Create(indexBufferInfo);


		VertexBufferCreateInfo vertexBufferInfo;
		vertexBufferInfo.Name = "Renderer2D_LineVB";
		vertexBufferInfo.Data = nullptr;
		vertexBufferInfo.Size = 1 * sizeof(LineVertex);
		vertexBufferInfo.IndexBuffer = nullptr;
		vertexBufferInfo.Flags = BufferMemoryFlags::CPU_WRITEABLE;

		m_LineVertexBuffer = VertexBuffer::Create(vertexBufferInfo);


		vertexBufferInfo.Name = "Renderer2D_CircleVB";
		vertexBufferInfo.Size = 1 * sizeof(CircleVertex);
		vertexBufferInfo.IndexBuffer = m_IndexBuffer.Get();

		m_CircleVertexBuffer = VertexBuffer::Create(vertexBufferInfo);


		vertexBufferInfo.Name = "Renderer2D_QuadVB";
		vertexBufferInfo.Size = 1 * sizeof(QuadVertex);
		vertexBufferInfo.IndexBuffer = m_IndexBuffer.Get();

		m_QuadVertexBuffer = VertexBuffer::Create(vertexBufferInfo);


		vertexBufferInfo.Name = "Renderer2D_TextVB";
		vertexBufferInfo.Size = 1 * sizeof(TextVertex);
		vertexBufferInfo.IndexBuffer = m_IndexBuffer.Get();

		m_TextVertexBuffer = VertexBuffer::Create(vertexBufferInfo);


		m_TextureSlots[0] = TextureGenerator::GetWhiteTexture();

		m_QuadVertexPositions[0] = { -0.5f,0.5f, 0.f, 1.f };
		m_QuadVertexPositions[1] = { 0.5f, 0.5f, 0.f, 1.f };
		m_QuadVertexPositions[2] = { 0.5f, -0.5f, 0.f, 1.f };
		m_QuadVertexPositions[3] = { -0.5f, -0.5f, 0.f, 1.f };

		
		PipelineCreateInfo pipelineBase;
		pipelineBase.RenderPass = renderPass;
		pipelineBase.CullMode = CullMode::NONE;
		pipelineBase.DepthCompareOp = DepthCompareOperator::GREATER_OR_EQUAL;
		pipelineBase.DepthWrite = false;
		pipelineBase.BlendEnable = true;


		PipelineCreateInfo quadPipeline = pipelineBase;
		quadPipeline.Name = "QuadPipeline";
		quadPipeline.Shader = Renderer::GetShaderPack()->Get("Renderer2D_Quad");
		quadPipeline.VertexLayout = {
			{ ShaderDataType::Float4, "a_Position" },
			{ ShaderDataType::Float4, "a_Color"    },
			{ ShaderDataType::Float2, "a_TexCoords"},
			{ ShaderDataType::UInt,   "a_TexIndex" }};
		quadPipeline.Topology = Topology::TRIANGLE_LIST;

		m_QuadPipeline = Pipeline::Create(quadPipeline);
		m_QuadPipeline->Bake();

		m_QuadBatchIndex = 0;
		QuadBatch quadBatch;
		quadBatch.IndexCount = 0;
		quadBatch.VertexOffset = 0;
		quadBatch.Material = Material::Create(m_QuadPipeline->GetInfo().Shader, std::format("Renderer2D_Quad_{}", m_QuadBatchIndex));

		m_QuadBatches.push_back(quadBatch);


		PipelineCreateInfo circlePipeline = pipelineBase;
		circlePipeline.Name = "CirclePipeline";
		circlePipeline.Shader = Renderer::GetShaderPack()->Get("Renderer2D_Circle");
		circlePipeline.VertexLayout = {
			{ ShaderDataType::Float4, "a_WorldPosition" },
			{ ShaderDataType::Float4, "a_Color"         },
			{ ShaderDataType::Float3, "a_LocalPosition" },
			{ ShaderDataType::Float,  "a_Thickness"     },
			{ ShaderDataType::Float,  "a_Fade"          }};
		circlePipeline.Topology = Topology::TRIANGLE_LIST;

		m_CirclePipeline = Pipeline::Create(circlePipeline);
		m_CirclePipeline->Bake();


		PipelineCreateInfo linePipeline = pipelineBase;
		linePipeline.Name = "LinePipeline";
		linePipeline.Shader = Renderer::GetShaderPack()->Get("Renderer2D_Line");
		linePipeline.VertexLayout = {
			{ ShaderDataType::Float4, "a_Position" },
			{ ShaderDataType::Float4, "a_Color"    }};
		linePipeline.Topology = Topology::LINE_LIST;

		m_LinePipeline = Pipeline::Create(linePipeline);
		m_LinePipeline->Bake();

		m_LineWidth = 1.f;
		m_LineBatchIndex = 0;
		LineBatch lineBatch;
		lineBatch.VertexCount = 0;
		lineBatch.VertexOffset = 0;
		lineBatch.LineWidth = m_LineWidth;

		m_LineBatches.push_back(lineBatch);


		PipelineCreateInfo textPipeline = pipelineBase;
		textPipeline.Name = "TextPipeline";
		textPipeline.Shader = Renderer::GetShaderPack()->Get("Renderer2D_Text");
		textPipeline.VertexLayout = {
			{ ShaderDataType::Float4, "a_Position" },
			{ ShaderDataType::Float4, "a_Color"    },
			{ ShaderDataType::Float2, "a_TexCoords"} };
		textPipeline.Topology = Topology::TRIANGLE_LIST;

		m_TextPipeline = Pipeline::Create(textPipeline);
		m_TextPipeline->Bake();

		m_TextBatchIndex = 0;
		TextBatch textBatch;
		textBatch.IndexCount = 0;
		textBatch.VertexOffset = 0;
		textBatch.Material = Material::Create(m_TextPipeline->GetInfo().Shader, std::format("Renderer2D_Text_{}", m_TextBatchIndex));

		m_TextBatches.push_back(textBatch);
	}

	void SceneRenderer2D::Shutdown()
	{

	}

	void SceneRenderer2D::OnViewportResize(uint32 width, uint32 height)
	{
		m_ViewportSize = Vector2u(width, height);

		m_QuadPipeline->SetViewport(width, height);
		m_LinePipeline->SetViewport(width, height);
		m_CirclePipeline->SetViewport(width, height);
		m_TextPipeline->SetViewport(width, height);
	}

	void SceneRenderer2D::BeginScene(const Matrix4& viewMatrix, const Matrix4& projectionMatrix)
	{
		m_BeginScene = true;
		m_RenderCommandBuffer = Renderer::GetRenderCommandBuffer();

		Matrix4 viewProj = viewMatrix * projectionMatrix;

		m_ViewProjection = viewProj;
		m_InverseView = Math::AffineInverse(viewMatrix);
		m_CameraPos = m_InverseView[3];

		const float aspectRatio = 16.f / 9.f;
		const float size = 10.f;
		m_OrthoViewProjection = Math::Ortho(0.f, aspectRatio * size, 0.f, size, size, 0.f);	// z will be equal 1

		for (auto& quadBatch : m_QuadBatches)
		{
			quadBatch.IndexCount = 0;
		}

		m_QuadBatchIndex = 0;
		m_TextureSlotIndex = 1;

		m_CircleIndexCount = 0;

		for (auto& lineBatch : m_LineBatches)
			lineBatch.VertexCount = 0;

		m_LineBatchIndex = 0;

		for (auto& textBatch : m_TextBatches)
			textBatch.IndexCount = 0;

		m_TextBatchIndex = 0;
		m_CurrentFont = nullptr;
	}

	void SceneRenderer2D::EndScene()
	{
		ATN_PROFILE_FUNC();

		FlushQuads();
		FlushLines();
		FlushText();

		FlushIndexBuffer();

		auto commandBuffer = m_RenderCommandBuffer;

		// QUADS
		{
			if (!m_QuadBatches.empty() && m_QuadBatches[0].IndexCount > 0)
			{
				m_QuadVertexBuffer.Flush();
				m_QuadPipeline->Bind(commandBuffer);
			}

			for (const auto& quadBatch : m_QuadBatches)
			{
				if (quadBatch.IndexCount > 0)
				{
					quadBatch.Material->Bind(commandBuffer);

					Renderer::RenderGeometry(commandBuffer, m_QuadPipeline, m_QuadVertexBuffer.Get(),
						quadBatch.Material, quadBatch.VertexOffset, quadBatch.IndexCount);
				}
			}
		}

		// CIRCLES
		if (m_CircleIndexCount != 0)
		{
			m_CircleVertexBuffer.Flush();
			m_CirclePipeline->Bind(commandBuffer);

			Renderer::RenderGeometry(commandBuffer, m_CirclePipeline, m_CircleVertexBuffer.Get(), 
				nullptr, 0, m_CircleIndexCount);
		}

		// LINES
		{
			if (!m_LineBatches.empty() && m_LineBatches[0].VertexCount > 0)
			{
				m_LineVertexBuffer.Flush();
				m_LinePipeline->Bind(commandBuffer);
			}

			for (const auto& lineBatch : m_LineBatches)
			{
				if (lineBatch.VertexCount > 0)
				{
					m_LinePipeline->SetLineWidth(commandBuffer, lineBatch.LineWidth);

					Renderer::RenderGeometry(commandBuffer, m_LinePipeline, m_LineVertexBuffer.Get(),
						nullptr, lineBatch.VertexOffset, lineBatch.VertexCount);
				}
			}
		}

		// TEXT
		{
			if (!m_TextBatches.empty() && m_TextBatches[0].IndexCount > 0)
			{
				m_TextVertexBuffer.Flush();
				m_TextPipeline->Bind(commandBuffer);
			}

			for (const auto& textBatch : m_TextBatches)
			{
				if (textBatch.IndexCount > 0)
				{
					textBatch.Material->Bind(commandBuffer);

					Renderer::RenderGeometry(commandBuffer, m_TextPipeline, m_TextVertexBuffer.Get(),
						textBatch.Material, textBatch.VertexOffset, textBatch.IndexCount);
				}
			}
		}

		m_BeginScene = false;
	}

	void SceneRenderer2D::FlushIndexBuffer()
	{
		uint64 quadIndices = 0;
		for(const auto& draw : m_QuadBatches)
			quadIndices += draw.IndexCount;

		uint64 textIndices = 0;
		for (const auto& draw : m_TextBatches)
			textIndices += draw.IndexCount;

		uint64 maxIndices = Math::Max(m_CircleIndexCount, quadIndices, textIndices);

		if (m_IndicesCount[Renderer::GetCurrentFrameIndex()] < maxIndices)
		{
			uint32* indices = new uint32[maxIndices];
			uint32 offset = 0;
			for (uint32 i = 0; i < maxIndices; i += 6)
			{
				indices[i + 0] = offset + 0;
				indices[i + 1] = offset + 1;
				indices[i + 2] = offset + 2;

				indices[i + 3] = offset + 2;
				indices[i + 4] = offset + 3;
				indices[i + 5] = offset + 0;

				offset += 4;
			}

			m_IndexBuffer.Push(indices, maxIndices * sizeof(uint32));
			m_IndexBuffer.Flush();

			delete[] indices;
		}

		m_IndicesCount[Renderer::GetCurrentFrameIndex()] = maxIndices;
	}

	void SceneRenderer2D::FlushQuads()
	{
		const QuadBatch& batch = m_QuadBatches[m_QuadBatchIndex];
		if (batch.IndexCount == 0)
			return;

		for (uint32 i = 0; i < m_TextureSlotIndex; ++i)
			batch.Material->Set("u_Textures", m_TextureSlots[i], i);

		m_TextureSlotIndex = 1;
	}

	void SceneRenderer2D::NextBatchQuads()
	{
		m_QuadBatchIndex++;

		// Offset for next batch
		const QuadBatch& prevBatch = m_QuadBatches[m_QuadBatchIndex - 1];
		uint32 vertexOffset = prevBatch.VertexOffset + 4 * prevBatch.IndexCount / 6;

		// If batch already exists update it offset
		// If not - create new batch
		if (m_QuadBatches.size() <= m_QuadBatchIndex)
		{
			QuadBatch batch;
			batch.IndexCount = 0;
			batch.VertexOffset = vertexOffset;

			batch.Material = Material::Create(m_QuadPipeline->GetInfo().Shader, std::format("Renderer2D_Quad_{}", m_QuadBatchIndex));
			batch.Material->Set("u_ViewProjection", m_ViewProjection);

			m_QuadBatches.push_back(batch);
		}
		else
		{
			m_QuadBatches[m_QuadBatchIndex].VertexOffset = vertexOffset;
		}
	}

	void SceneRenderer2D::FlushLines()
	{
		LineBatch& batch = m_LineBatches[m_LineBatchIndex];
		batch.LineWidth = m_LineWidth;
	}

	void SceneRenderer2D::NextBatchLines()
	{
		m_LineBatchIndex++;

		// Offset for next batch
		const LineBatch& prevBatch = m_LineBatches[m_LineBatchIndex - 1];
		uint32 vertexOffset = prevBatch.VertexOffset + prevBatch.VertexCount;

		// If batch already exists update it offset
		// If not - create new batch
		if (m_LineBatches.size() <= m_LineBatchIndex)
		{
			LineBatch batch;
			batch.VertexCount = 0;
			batch.VertexOffset = vertexOffset;

			m_LineBatches.push_back(batch);
		}
		else
		{
			m_LineBatches[m_LineBatchIndex].VertexOffset = vertexOffset;
		}
	}

	void SceneRenderer2D::FlushText()
	{
		const TextBatch& batch = m_TextBatches[m_TextBatchIndex];
		if (batch.IndexCount == 0)
			return;

		batch.Material->Set("u_AtlasTexture", m_CurrentFont->GetAtlasTexture());
	}

	void SceneRenderer2D::NextBatchText()
	{
		m_TextBatchIndex++;

		// Offset for next batch
		const TextBatch& prevBatch = m_TextBatches[m_TextBatchIndex - 1];
		uint32 vertexOffset = prevBatch.VertexOffset + 4 * prevBatch.IndexCount / 6;

		// If batch already exists update it offset
		// If not - create new batch
		if (m_TextBatches.size() <= m_TextBatchIndex)
		{
			TextBatch batch;
			batch.IndexCount = 0;
			batch.VertexOffset = vertexOffset;

			batch.Material = Material::Create(m_TextPipeline->GetInfo().Shader, std::format("Renderer2D_Text_{}", m_TextBatchIndex));

			m_TextBatches.push_back(batch);
		}
		else
		{
			m_TextBatches[m_TextBatchIndex].VertexOffset = vertexOffset;
		}
	}

	void SceneRenderer2D::DrawQuad(Vector2 position, Vector2 size, const LinearColor& color)
	{
		DrawQuad({ position.x, position.y, 0.f }, size, color);
	}

	void SceneRenderer2D::DrawQuad(Vector3 position, Vector2 size, const LinearColor& color)
	{
		Matrix4 transform = Math::ScaleMatrix(Vector3(size.x, size.y, 1.f)).Translate(position);

		DrawQuad(transform, Renderer2DSpace::WorldSpace, color);
	}

	void SceneRenderer2D::DrawQuad(Vector2 position, Vector2 size, const Texture2DInstance& texture, const LinearColor& tint, float tilingFactor)
	{
		DrawQuad({ position.x, position.y, 0.f }, size, texture, tint, tilingFactor);
	}

	void SceneRenderer2D::DrawQuad(Vector3 position, Vector2 size, const Texture2DInstance& texture, const LinearColor& tint, float tilingFactor)
	{
		Matrix4 transform = ScaleMatrix(Vector3(size.x, size.y, 1.f)).Translate(position);

		DrawQuad(transform, texture, Renderer2DSpace::WorldSpace, tint, tilingFactor);
	}

	void SceneRenderer2D::DrawRotatedQuad(Vector2 position, Vector2 size, float rotation, const LinearColor& color)
	{
		DrawRotatedQuad({ position.x, position.y, 0.f }, size, rotation, color);
	}

	void SceneRenderer2D::DrawRotatedQuad(Vector3 position, Vector2 size, float rotation, const LinearColor& color)
	{
		Matrix4 transform =
			Math::ScaleMatrix(Vector3(size.x, size.y, 1.f)).Rotate(rotation, Vector3(0.f, 0.f, 1.f)).Translate(position);

		DrawQuad(transform, Renderer2DSpace::WorldSpace, color);
	}

	void SceneRenderer2D::DrawRotatedQuad(Vector2 position, Vector2 size, float rotation, const Texture2DInstance& texture, const LinearColor& tint, float tilingFactor)
	{
		DrawRotatedQuad({ position.x, position.y, 0.f }, size, rotation, texture, tint, tilingFactor);
	}

	void SceneRenderer2D::DrawRotatedQuad(Vector3 position, Vector2 size, float rotation, const Texture2DInstance& texture, const LinearColor& tint, float tilingFactor)
	{
		Matrix4 transform =
			Math::ScaleMatrix(Vector3(size.x, size.y, 1.f)).Rotate(rotation, Vector3(0.f, 0.f, 1.f)).Translate(position);

		DrawQuad(transform, texture, Renderer2DSpace::WorldSpace, tint, tilingFactor);
	}

	void SceneRenderer2D::DrawQuad(const Matrix4& worldTransform, Renderer2DSpace space, const LinearColor& color)
	{
		const uint32 textureIndex = 0; // White Texture
		const Vector2 textureCoords[4] = { {0.f, 0.f}, {1.f, 0.f}, {1.f, 1.f}, {0.f, 1.f} };
		const float tilingFactor = 1.f;

		Matrix4 transform = GetSpaceTransform(worldTransform, space);

		QuadVertex vertices[4];

		for (uint32 i = 0; i < 4; ++i)
		{
			vertices[i].Position = m_QuadVertexPositions[i] * transform;
			vertices[i].Color = color;
			vertices[i].TexCoords = textureCoords[i] * tilingFactor;
			vertices[i].TexIndex = textureIndex;
		}

		m_QuadVertexBuffer.Push(vertices, sizeof(vertices));
		m_QuadBatches[m_QuadBatchIndex].IndexCount += 6;
	}

	void SceneRenderer2D::DrawQuad(const Matrix4& worldTransform, const Texture2DInstance& texture, Renderer2DSpace space, const LinearColor& tint, float tilingFactor)
	{
		const auto& texCoords = texture.GetTexCoords();
		int32 textureIndex = 0;

		for (uint32 i = 1; i < m_TextureSlotIndex; ++i)
		{
			if (m_TextureSlots[i] == texture.GetNativeTexture())
			{
				textureIndex = i;
				break;
			}
		}

		if (textureIndex == 0)
		{
			if (m_TextureSlotIndex >= s_MaxTextureSlots)
			{
				FlushQuads();
				NextBatchQuads();
			}
			
			textureIndex = m_TextureSlotIndex;
			m_TextureSlots[m_TextureSlotIndex] = texture.GetNativeTexture();
			m_TextureSlotIndex++;
		}

		Matrix4 transform = GetSpaceTransform(worldTransform, space);

		QuadVertex vertices[4];

		for (uint32 i = 0; i < 4; ++i)
		{
			vertices[i].Position = m_QuadVertexPositions[i] * transform;
			vertices[i].Color = tint;
			vertices[i].TexCoords = texCoords[i] * tilingFactor;
			vertices[i].TexIndex = textureIndex;
		}

		m_QuadVertexBuffer.Push(vertices, sizeof(vertices));
		m_QuadBatches[m_QuadBatchIndex].IndexCount += 6;
	}

	void SceneRenderer2D::DrawBillboardFixedSize(const Vector3& position, Vector2 size, const LinearColor& color)
	{
		float distance = Math::Distance(m_CameraPos, position);
		size *= distance;
		Matrix4 transform = Math::ConstructTransform(position, { size.x, size.y, 1 }, Quaternion(1, 0, 0, 0) * m_InverseView);
		DrawQuad(transform, Renderer2DSpace::WorldSpace, color);
	}

	void SceneRenderer2D::DrawBillboardFixedSize(const Vector3& position, Vector2 size, const Texture2DInstance& texture, const LinearColor& tint, float tilingFactor)
	{
		float distance = Math::Distance(m_CameraPos, position);
		size *= distance;
		Matrix4 transform = Math::ConstructTransform(position, { size.x, size.y, 1 }, Quaternion(1, 0, 0, 0) * m_InverseView);
		DrawQuad(transform, texture, Renderer2DSpace::WorldSpace, tint, tilingFactor);
	}

	void SceneRenderer2D::DrawCircle(const Matrix4& worldTransform, Renderer2DSpace space, const LinearColor& color, float thickness, float fade)
	{
		Matrix4 transform = GetSpaceTransform(worldTransform, space);

		CircleVertex vertices[4];

		for (uint32 i = 0; i < 4; ++i)
		{
			vertices[i].WorldPosition = m_QuadVertexPositions[i] * transform;
			vertices[i].LocalPosition = m_QuadVertexPositions[i] * 2.f;
			vertices[i].Color = color;
			vertices[i].Thickness = thickness;
			vertices[i].Fade = fade;
		}

		m_CircleVertexBuffer.Push(vertices, sizeof(vertices));
		m_CircleIndexCount += 6;
	}

	void SceneRenderer2D::DrawLine(const Vector3& p0, const Vector3& p1, const LinearColor& color)
	{
		LineVertex vertices[2];

		vertices[0].Position = Vector4(p0, 1.0) * m_ViewProjection;
		vertices[0].Color = color;

		vertices[1].Position = Vector4(p1, 1.0) * m_ViewProjection;
		vertices[1].Color = color;

		m_LineVertexBuffer.Push(vertices, sizeof(vertices));
		m_LineBatches[m_LineBatchIndex].VertexCount += 2;
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

	void SceneRenderer2D::DrawText(const String& text, const Ref<Font>& font, const Matrix4& worldTransform, Renderer2DSpace space, const TextParams& params)
	{
		if (text.empty() || text[0] == '\0')
			return;

		if (m_CurrentFont == nullptr && m_TextBatches[m_TextBatchIndex].IndexCount == 0)
			m_CurrentFont = font;

		if (m_CurrentFont != font)
		{
			FlushText();
			NextBatchText();
			m_CurrentFont = font;
		}

		Matrix4 transform = GetSpaceTransform(worldTransform, space);

		std::u32string unicodeText = Utils::ToUTF32String(text);

		Ref<Texture2D> atlasTexture = font->GetAtlasTexture();
		float texelWidth = 1.0f / atlasTexture->GetWidth();
		float texelHeight = 1.0f / atlasTexture->GetHeight();

		const auto& fontGeometry = font->GetAtlasGeometryData()->FontGeometry;
		const auto& metrics = fontGeometry.getMetrics();

		double fsScale = 1.0 / (metrics.ascenderY - metrics.descenderY);
		double x = 0.0;
		double y = space == Renderer2DSpace::ScreenSpace ? fsScale * metrics.lineHeight + params.LineSpacing : 0.0;

		const float spaceGlyphAdvance = fontGeometry.getGlyph(' ')->getAdvance();
		double newLineAdvance = -(fsScale * metrics.lineHeight + params.LineSpacing);

		if (space == Renderer2DSpace::ScreenSpace)
			newLineAdvance = -newLineAdvance;

		for (uint32 i = 0; i < unicodeText.size(); ++i)
		{
			char32_t character = unicodeText[i];

			if (character == '\0')
				break;

			if (character == '\r')
				continue;

			if (character == '\n')
			{
				x = 0;
				y += newLineAdvance;
				continue;
			}

			if (character == ' ')
			{
				float advance;

				if (i < unicodeText.size() - 1)
				{
					char32_t nextCharacter = unicodeText[i + 1];
					double dAdvance;
					fontGeometry.getAdvance(dAdvance, character, nextCharacter);
					advance = (float)dAdvance;
				}
				else
				{
					advance = spaceGlyphAdvance;
				}

				x += fsScale * advance + params.Kerning;
				continue;
			}

			const msdf_atlas::GlyphGeometry* glyph = fontGeometry.getGlyph(character);

			if (!glyph)
				glyph = fontGeometry.getGlyph('?');

			if (!glyph)
				continue;

			double al, ab, ar, at;
			glyph->getQuadAtlasBounds(al, ab, ar, at);
			Vector2 texCoordMin((float)al, (float)ab);
			Vector2 texCoordMax((float)ar, (float)at);

			texCoordMin *= Vector2(texelWidth, texelHeight);
			texCoordMax *= Vector2(texelWidth, texelHeight);

			double pl, pb, pr, pt;
			glyph->getQuadPlaneBounds(pl, pb, pr, pt);
			Vector2 quadMin((float)pl, (float)pb);
			Vector2 quadMax((float)pr, (float)pt);

			if (space == Renderer2DSpace::ScreenSpace)
			{
				quadMin.y = -quadMin.y;
				quadMax.y = -quadMax.y;
			}

			quadMin *= fsScale, quadMax *= fsScale;
			quadMin += Vector2(x, y);
			quadMax += Vector2(x, y);

			TextVertex vertices[4];
			
			vertices[0].Position = Vector4(quadMin, 0.0f, 1.0f) * transform;
			vertices[0].Color = params.Color;
			vertices[0].TexCoords = texCoordMin;

			vertices[1].Position = Vector4(quadMin.x, quadMax.y, 0.0f, 1.0f) * transform;
			vertices[1].Color = params.Color;
			vertices[1].TexCoords = { texCoordMin.x, texCoordMax.y };

			vertices[2].Position = Vector4(quadMax, 0.0f, 1.0f) * transform;
			vertices[2].Color = params.Color;
			vertices[2].TexCoords = texCoordMax;

			vertices[3].Position = Vector4(quadMax.x, quadMin.y, 0.0f, 1.0f) * transform;
			vertices[3].Color = params.Color;
			vertices[3].TexCoords = { texCoordMax.x, texCoordMin.y };

			m_TextVertexBuffer.Push(vertices, sizeof(vertices));
			m_TextBatches[m_TextBatchIndex].IndexCount += 6;

			if (i < unicodeText.size() - 1)
			{
				double advance = glyph->getAdvance();
				char32_t nextCharacter = unicodeText[i + 1];
				fontGeometry.getAdvance(advance, character, nextCharacter);

				x += fsScale * advance + params.Kerning;
			}
		}
	}

	void SceneRenderer2D::DrawScreenSpaceText(const String& text, const Ref<Font>& font, Vector2 position, Vector2 scale, const TextParams& params)
	{
		Matrix4 transform = Math::TranslateMatrix(Vector3(position, 1.0)).Scale(Vector3(scale, 1.0));
		DrawText(text, font, transform, Renderer2DSpace::ScreenSpace, params);
	}

	void SceneRenderer2D::SetLineWidth(float width)
	{
		if (m_BeginScene && m_LineBatches[m_LineBatchIndex].VertexCount != 0)
		{
			FlushLines();
			NextBatchLines();
		}

		m_LineWidth = width;
	}

	float SceneRenderer2D::GetLineWidth()
	{
		return m_LineWidth;
	}

	Matrix4 SceneRenderer2D::GetSpaceTransform(const Matrix4& worldTransform, Renderer2DSpace space)
	{
		Matrix4 transform;
		if (space == Renderer2DSpace::WorldSpace)
		{
			transform = worldTransform * m_ViewProjection;
		}
		else if (space == Renderer2DSpace::ScreenSpace)
		{
			transform = worldTransform * m_OrthoViewProjection;
		}
		else if (space == Renderer2DSpace::Billboard) // TODO
		{
			Vector3 translation, rotation, scale;
			Math::DecomposeTransform(worldTransform, translation, rotation, scale);

			Quaternion newRotation = Quaternion(1.0, 0.f, 0.f, 0.0) * m_InverseView;

			transform = Math::ConstructTransform(translation, scale, newRotation);
			transform = transform * m_ViewProjection;
		}

		return transform;
	}
}
