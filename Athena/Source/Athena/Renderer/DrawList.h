#pragma once 

#include "Athena/Core/Core.h"
#include "Athena/Math/Matrix.h"
#include "Athena/Renderer/Animation.h"
#include "Athena/Renderer/GPUBuffer.h"
#include "Athena/Renderer/Material.h"
#include "Athena/Renderer/Pipeline.h"

#include <deque>


namespace Athena
{
	struct InstanceTransformData
	{
		Vector3 TRow0;
		Vector3 TRow1;
		Vector3 TRow2;
		Vector3 TRow3;
	};

	struct StaticDrawCall
	{
		Ref<VertexBuffer> MeshVertexBuffer;
		Ref<IndexBuffer> MeshIndexBuffer;

		uint32 BaseIndex;
		uint32 IndexCount;
		uint32 BaseVertex;
		uint32 VertexCount;

		Ref<Material> Material;
		Matrix4 Transform;
	};

	class ATHENA_API DrawListStatic
	{
	public:
		void Push(const StaticDrawCall& drawCall);
		void Sort();

		void Flush(const Ref<RenderCommandBuffer> commandBuffer, const Ref<Pipeline>& pipeline);
		void FlushNoMaterials(const Ref<RenderCommandBuffer> commandBuffer, const Ref<Pipeline>& pipeline, bool shadowPass = false);

		void SetInstanceOffset(uint32 offset) { m_InstanceOffset = offset; }
		void EmplaceInstanceTransforms(std::vector<InstanceTransformData>& data);

		uint32 GetInstancesCount() const;

		uint64 Size() const { return m_Array.size(); }
		void Clear();

	private:
		std::vector<StaticDrawCall> m_Array;
		uint32 m_InstanceOffset = 0;
	};

	struct AnimDrawCall
	{
		Ref<VertexBuffer> MeshVertexBuffer;
		Ref<IndexBuffer> MeshIndexBuffer;
		Ref<VertexBuffer> BonesInfluenceBuffer;
		uint32 BonesOffset;

		uint32 BaseIndex;
		uint32 IndexCount;
		uint32 BaseVertex;
		uint32 VertexCount;

		Ref<Material> Material;
		Matrix4 Transform;
	};

	class ATHENA_API DrawListAnim
	{
	public:
		void Push(const AnimDrawCall& drawCall);
		void Sort();

		void Flush(const Ref<RenderCommandBuffer> commandBuffer, const Ref<Pipeline>& pipeline);
		void FlushNoMaterials(const Ref<RenderCommandBuffer> commandBuffer, const Ref<Pipeline>& pipeline, bool shadowPass = false);

		void SetInstanceOffset(uint32 offset) { m_InstanceOffset = offset; }
		void EmplaceInstanceTransforms(std::vector<InstanceTransformData>& data);

		uint64 Size() const { return m_Array.size(); }
		void Clear();

		const auto& GetArray() const { return m_Array; }

	private:
		std::vector<AnimDrawCall> m_Array;
		uint32 m_InstanceOffset = 0;
	};
}
