#include "DrawList.h"

#include "Athena/Renderer/Material.h"
#include "Athena/Renderer/Renderer.h"
#include "Athena/Math/Common.h"


namespace Athena
{
	void DrawListStatic::Push(const StaticDrawCall& drawCall)
	{
		m_Array.push_back(drawCall);
	}

	void DrawListStatic::Clear()
	{
		m_Array.clear();
	}

	void DrawListStatic::Sort()
	{
		ATN_PROFILE_FUNC();

		std::sort(m_Array.begin(), m_Array.end(), [](const StaticDrawCall& left, const StaticDrawCall& right)
		{
			return std::tie(left.MeshVertexBuffer, left.Material, left.BaseVertex) <
				std::tie(right.MeshVertexBuffer, right.Material, right.BaseVertex);
		});
	}

	void DrawListStatic::Flush(const Ref<RenderCommandBuffer> commandBuffer, const Ref<Pipeline>& pipeline)
	{
		ATN_PROFILE_FUNC();

		if (m_Array.empty())
			return;

		const auto& first = m_Array[0];
		first.Material->Bind(commandBuffer);
		Renderer::BindGeometryBuffers(commandBuffer, first.MeshVertexBuffer, first.MeshIndexBuffer);

		uint32 instanceOffset = m_InstanceOffset;
		uint32 instanceCount = 1;

		for (uint32 i = 1; i < m_Array.size(); ++i)
		{
			const auto& current = m_Array[i];
			const auto& previous = m_Array[i - 1];

			if (current.MeshVertexBuffer == previous.MeshVertexBuffer && current.Material == previous.Material && current.BaseVertex == previous.BaseVertex)
			{
				instanceCount++;
				continue;
			}

			Renderer::RenderGeometryInstanced(commandBuffer, pipeline, previous.Material, previous.BaseIndex, previous.IndexCount,
				previous.BaseVertex, previous.VertexCount, instanceCount, instanceOffset);

			if (current.Material != previous.Material)
				current.Material->Bind(commandBuffer);

			if (current.MeshVertexBuffer != previous.MeshVertexBuffer)
				Renderer::BindGeometryBuffers(commandBuffer, current.MeshVertexBuffer, current.MeshIndexBuffer);

			instanceOffset += instanceCount;
			instanceCount = 1;
		}

		const auto& last = m_Array.back();
		Renderer::RenderGeometryInstanced(commandBuffer, pipeline, last.Material, last.BaseIndex, last.IndexCount,
			last.BaseVertex, last.VertexCount, instanceCount, instanceOffset);

#if OLD
		Ref<VertexBuffer> instanceVertexBuffer = m_Array[0].MeshVertexBuffer;
		Ref<Material> instanceMaterial = m_Array[0].Material;
		uint32 instanceBaseVertex = m_Array[0].BaseVertex;

		instanceMaterial->Bind(commandBuffer);
		Renderer::BindGeometryBuffers(commandBuffer, instanceVertexBuffer, m_Array[0].MeshIndexBuffer);

		uint32 instanceOffset = m_InstanceOffset;
		uint32 instanceCount = 0;

		for (const auto& drawCall : m_Array)
		{
			if (drawCall.Material == instanceMaterial && drawCall.MeshVertexBuffer == instanceVertexBuffer && drawCall.BaseVertex == instanceBaseVertex)
			{
				instanceCount++;
				continue;
			}

			const auto& instanceDrawCall = m_Array.back();
			Renderer::RenderGeometryInstanced(commandBuffer, pipeline, instanceDrawCall.Material, instanceDrawCall.BaseIndex, instanceDrawCall.IndexCount,
				instanceDrawCall.BaseVertex, instanceDrawCall.VertexCount, instanceCount, instanceOffset);

			if (drawCall.Material != instanceMaterial)
				drawCall.Material->Bind(commandBuffer);

			if (drawCall.MeshVertexBuffer != instanceVertexBuffer)
				Renderer::BindGeometryBuffers(commandBuffer, drawCall.MeshVertexBuffer, drawCall.MeshIndexBuffer);

			instanceOffset += instanceCount + 1;
			instanceCount = 1;
			instanceVertexBuffer = drawCall.MeshVertexBuffer;
			instanceMaterial = drawCall.Material;
			instanceBaseVertex = drawCall.BaseVertex;
		}

		const auto& instanceDrawCall = m_Array.back();
		Renderer::RenderGeometryInstanced(commandBuffer, pipeline, instanceDrawCall.Material, instanceDrawCall.BaseIndex, instanceDrawCall.IndexCount,
			instanceDrawCall.BaseVertex, instanceDrawCall.VertexCount, instanceCount, instanceOffset);

#endif
	}

	void DrawListStatic::FlushNoMaterials(const Ref<RenderCommandBuffer> commandBuffer, const Ref<Pipeline>& pipeline, bool shadowPass)
	{
		ATN_PROFILE_FUNC();

		if (m_Array.empty())
			return;

		const auto& first = m_Array[0];
		Renderer::BindGeometryBuffers(commandBuffer, first.MeshVertexBuffer, first.MeshIndexBuffer);

		uint32 instanceOffset = m_InstanceOffset;
		uint32 instanceCount = 1;

		for (uint32 i = 1; i < m_Array.size(); ++i)
		{
			const auto& current = m_Array[i];
			const auto& previous = m_Array[i - 1];

			bool render = shadowPass ? current.Material->IsFlagSet(MaterialFlag::CastShadows) : true;

			if (current.MeshVertexBuffer == previous.MeshVertexBuffer && current.BaseVertex == previous.BaseVertex && render)
			{
				instanceCount++;
				continue;
			}

			if (instanceCount != 0)
			{
				Renderer::RenderGeometryInstanced(commandBuffer, pipeline, nullptr, previous.BaseIndex, previous.IndexCount,
					previous.BaseVertex, previous.VertexCount, instanceCount, instanceOffset);
			}

			if (current.MeshVertexBuffer != previous.MeshVertexBuffer)
				Renderer::BindGeometryBuffers(commandBuffer, current.MeshVertexBuffer, current.MeshIndexBuffer);

			if (render)
			{
				instanceOffset += instanceCount;
				instanceCount = 1;
			}
			else
			{
				instanceOffset += instanceCount + 1;
				instanceCount = 0;
			}
		}

		const auto& last = m_Array.back();
		bool render = shadowPass ? last.Material->IsFlagSet(MaterialFlag::CastShadows) : true;
		if (render)
		{
			Renderer::RenderGeometryInstanced(commandBuffer, pipeline, nullptr, last.BaseIndex, last.IndexCount,
				last.BaseVertex, last.VertexCount, instanceCount, instanceOffset);
		}

#if OLD
		Ref<VertexBuffer> instanceVertexBuffer;
		uint32 instanceOffset = m_InstanceOffset;
		uint32 instanceCount = 0;

		for (const auto& drawCall : m_Array)
		{
			if (instanceCount == 0)
				instanceVertexBuffer = drawCall.MeshVertexBuffer;

			if (shadowPass && !drawCall.Material->IsFlagSet(MaterialFlag::CastShadows))
			{
				if(instanceCount != 0)
					Renderer::RenderGeometryInstanced(commandBuffer, pipeline, instanceVertexBuffer, nullptr, instanceCount, instanceOffset);

				instanceOffset += instanceCount + 1;
				instanceCount = 0;

				instanceVertexBuffer = drawCall.MeshVertexBuffer;
			}
			else if (drawCall.MeshVertexBuffer != instanceVertexBuffer)
			{
				Renderer::RenderGeometryInstanced(commandBuffer, pipeline, instanceVertexBuffer, nullptr, instanceCount, instanceOffset);
				instanceOffset += instanceCount;
				instanceCount = 1;

				instanceVertexBuffer = drawCall.MeshVertexBuffer;
			}
			else
			{
				instanceCount++;
			}
		}

		if (!m_Array.empty())
		{
			if((*(m_Array.end() - 1)).Material->IsFlagSet(MaterialFlag::CastShadows))
				Renderer::RenderGeometryInstanced(commandBuffer, pipeline, instanceVertexBuffer, nullptr, instanceCount, instanceOffset);
		}
#endif
	}

	void DrawListStatic::EmplaceInstanceTransforms(std::vector<InstanceTransformData>& data)
	{
		data.reserve(m_Array.size());

		for (const auto& draw : m_Array)
		{
			InstanceTransformData transformData;
			transformData.TRow0 = draw.Transform[0];
			transformData.TRow1 = draw.Transform[1];
			transformData.TRow2 = draw.Transform[2];
			transformData.TRow3 = draw.Transform[3];

			data.push_back(transformData);
		}
	}

	uint32 DrawListStatic::GetInstancesCount() const
	{
		uint32 instancesCount = 1;

		for (uint32 i = 1; i < m_Array.size(); ++i)
		{
			const auto& current = m_Array[i];
			const auto& previous = m_Array[i - 1];

			if (current.MeshVertexBuffer == previous.MeshVertexBuffer && current.Material == previous.Material && current.BaseVertex == previous.BaseVertex)
			{
				continue;
			}

			instancesCount++;
		}

		return instancesCount;
	}


	void DrawListAnim::Push(const AnimDrawCall& drawCall)
	{
		m_Array.push_back(drawCall);
	}

	void DrawListAnim::Clear()
	{
		m_Array.clear();
	}

	void DrawListAnim::Sort()
	{
		ATN_PROFILE_FUNC();

		std::sort(m_Array.begin(), m_Array.end(), [](const AnimDrawCall& left, const AnimDrawCall& right)
		{
			return std::tie(left.MeshVertexBuffer, left.Material, left.BaseVertex) <
				std::tie(right.MeshVertexBuffer, right.Material, right.BaseVertex);
		});
	}

	void DrawListAnim::Flush(const Ref<RenderCommandBuffer> commandBuffer, const Ref<Pipeline>& pipeline)
	{
		ATN_PROFILE_FUNC();

		uint32 instanceOffset = m_InstanceOffset;

		Ref<Material> instanceMaterial;
		Ref<VertexBuffer> instanceBuffer;

		for (const auto& drawCall : m_Array)
		{
			if (drawCall.Material != instanceMaterial)
			{
				instanceMaterial = drawCall.Material;
				instanceMaterial->Bind(commandBuffer);
			}

			if (drawCall.MeshVertexBuffer != instanceBuffer)
			{
				instanceBuffer = drawCall.MeshVertexBuffer;
				Renderer::BindGeometryBuffers(commandBuffer, drawCall.MeshVertexBuffer, drawCall.MeshIndexBuffer, drawCall.BonesInfluenceBuffer);
			}

			instanceMaterial->Set("u_BonesOffset", drawCall.BonesOffset);
			Renderer::RenderGeometryInstanced(commandBuffer, pipeline, instanceMaterial, drawCall.BaseIndex, drawCall.IndexCount, 
				drawCall.BaseVertex, drawCall.VertexCount, 1, instanceOffset);

			instanceOffset++;
		}
	}

	void DrawListAnim::FlushNoMaterials(const Ref<RenderCommandBuffer> commandBuffer, const Ref<Pipeline>& pipeline, bool shadowPass)
	{
		ATN_PROFILE_FUNC();

		uint32 instanceOffset = m_InstanceOffset;
		Ref<VertexBuffer> instanceBuffer;

		for (const auto& drawCall : m_Array)
		{
			if (shadowPass && !drawCall.Material->IsFlagSet(MaterialFlag::CastShadows))
			{
				instanceOffset++;
				continue;
			}

			if (drawCall.MeshVertexBuffer != instanceBuffer)
			{
				instanceBuffer = drawCall.MeshVertexBuffer;
				Renderer::BindGeometryBuffers(commandBuffer, drawCall.MeshVertexBuffer, drawCall.MeshIndexBuffer, drawCall.BonesInfluenceBuffer);
			}

			drawCall.Material->Set("u_BonesOffset", drawCall.BonesOffset);
			Renderer::RenderGeometryInstanced(commandBuffer, pipeline, drawCall.Material, drawCall.BaseIndex, drawCall.IndexCount,
				drawCall.BaseVertex, drawCall.VertexCount, 1, instanceOffset);
			
			instanceOffset++;
		}
	}

	void DrawListAnim::EmplaceInstanceTransforms(std::vector<InstanceTransformData>& data)
	{
		data.reserve(m_Array.size());

		for (const auto& draw : m_Array)
		{
			InstanceTransformData transformData;
			transformData.TRow0 = draw.Transform[0];
			transformData.TRow1 = draw.Transform[1];
			transformData.TRow2 = draw.Transform[2];
			transformData.TRow3 = draw.Transform[3];

			data.push_back(transformData);
		}
	}
}
