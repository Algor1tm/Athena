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
		// Sort by material and vertex buffer(for instancing)
		std::sort(m_Array.begin(), m_Array.end(), [](const StaticDrawCall& left, const StaticDrawCall& right)
		{
			const auto& leftName = left.Material->GetName();
			const auto& rightName = right.Material->GetName();

			if (leftName == rightName)
				return left.VertexBuffer.Raw() < right.VertexBuffer.Raw();

			return leftName < rightName;
		});
	}

	void DrawListStatic::Flush(const Ref<RenderCommandBuffer> commandBuffer, const Ref<Pipeline>& pipeline)
	{
		Ref<VertexBuffer> instanceVertexBuffer;
		Ref<Material> instanceMaterial;
		if (!m_Array.empty())
		{
			instanceVertexBuffer = m_Array[0].VertexBuffer;
			instanceMaterial = m_Array[0].Material;
			instanceMaterial->Bind(commandBuffer);
		}

		uint32 instanceOffset = m_InstanceOffset;
		uint32 instanceCount = 0;

		for (const auto& drawCall : m_Array)
		{
			// Flush instances if material changed or vertex buffer
			if (drawCall.Material != instanceMaterial)
			{
				Renderer::RenderGeometryInstanced(commandBuffer, pipeline, instanceVertexBuffer, instanceMaterial, instanceCount, instanceOffset);
				instanceOffset += instanceCount;

				instanceCount = 1;
				instanceVertexBuffer = drawCall.VertexBuffer;
				instanceMaterial = drawCall.Material;
				instanceMaterial->Bind(commandBuffer);
			}
			else if (drawCall.VertexBuffer != instanceVertexBuffer)
			{
				Renderer::RenderGeometryInstanced(commandBuffer, pipeline, instanceVertexBuffer, drawCall.Material, instanceCount, instanceOffset);
				instanceOffset += instanceCount;

				instanceCount = 1;
				instanceVertexBuffer = drawCall.VertexBuffer;
			}
			else
			{
				instanceCount++;
			}
		}

		if(!m_Array.empty())
			Renderer::RenderGeometryInstanced(commandBuffer, pipeline, instanceVertexBuffer, instanceMaterial, instanceCount, instanceOffset);
	}

	void DrawListStatic::FlushNoMaterials(const Ref<RenderCommandBuffer> commandBuffer, const Ref<Pipeline>& pipeline, bool shadowPass)
	{
		Ref<VertexBuffer> instanceVertexBuffer;
		uint32 instanceOffset = m_InstanceOffset;
		uint32 instanceCount = 0;

		for (const auto& drawCall : m_Array)
		{
			if (instanceCount == 0)
				instanceVertexBuffer = drawCall.VertexBuffer;

			if (shadowPass && !drawCall.Material->GetFlag(MaterialFlag::CAST_SHADOWS))
			{
				if(instanceCount != 0)
					Renderer::RenderGeometryInstanced(commandBuffer, pipeline, instanceVertexBuffer, nullptr, instanceCount, instanceOffset);

				instanceOffset += instanceCount + 1;
				instanceCount = 0;

				instanceVertexBuffer = drawCall.VertexBuffer;
			}
			else if (drawCall.VertexBuffer != instanceVertexBuffer)
			{
				Renderer::RenderGeometryInstanced(commandBuffer, pipeline, instanceVertexBuffer, nullptr, instanceCount, instanceOffset);
				instanceOffset += instanceCount;
				instanceCount = 1;

				instanceVertexBuffer = drawCall.VertexBuffer;
			}
			else
			{
				instanceCount++;
			}
		}

		if (!m_Array.empty())
		{
			if((*(m_Array.end() - 1)).Material->GetFlag(MaterialFlag::CAST_SHADOWS))
				Renderer::RenderGeometryInstanced(commandBuffer, pipeline, instanceVertexBuffer, nullptr, instanceCount, instanceOffset);
		}
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
		uint32 instances = 1;

		Ref<VertexBuffer> instanceVertexBuffer;
		Ref<Material> instanceMaterial;
		if (!m_Array.empty())
		{
			instanceVertexBuffer = m_Array[0].VertexBuffer;
			instanceMaterial = m_Array[0].Material;
		}

		for (const auto& drawCall : m_Array)
		{
			if (drawCall.Material != instanceMaterial || drawCall.VertexBuffer != instanceVertexBuffer)
				instances++;
		}

		return instances;
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
		// Sort by material
		std::sort(m_Array.begin(), m_Array.end(), [](const AnimDrawCall& left, const AnimDrawCall& right)
		{
			return left.Material->GetName() < right.Material->GetName();
		});
	}

	void DrawListAnim::Flush(const Ref<RenderCommandBuffer> commandBuffer, const Ref<Pipeline>& pipeline)
	{
		uint32 instanceOffset = m_InstanceOffset;

		Ref<Material> instanceMaterial;

		for (const auto& drawCall : m_Array)
		{
			if (drawCall.Material != instanceMaterial)
			{
				instanceMaterial = drawCall.Material;
				instanceMaterial->Bind(commandBuffer);
			}

			instanceMaterial->Set("u_BonesOffset", drawCall.BonesOffset);
			Renderer::RenderGeometryInstanced(commandBuffer, pipeline, drawCall.VertexBuffer, instanceMaterial, 1, instanceOffset);

			instanceOffset++;
		}
	}

	void DrawListAnim::FlushNoMaterials(const Ref<RenderCommandBuffer> commandBuffer, const Ref<Pipeline>& pipeline, bool shadowPass)
	{
		uint32 instanceOffset = m_InstanceOffset;

		for (const auto& drawCall : m_Array)
		{
			if (shadowPass && !drawCall.Material->GetFlag(MaterialFlag::CAST_SHADOWS))
				continue;

			drawCall.Material->Set("u_BonesOffset", drawCall.BonesOffset);
			Renderer::RenderGeometryInstanced(commandBuffer, pipeline, drawCall.VertexBuffer, drawCall.Material, 1, instanceOffset);
			
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
