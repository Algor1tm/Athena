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
	struct StaticDrawCall
	{
		Ref<VertexBuffer> VertexBuffer;
		Ref<Material> Material;
		Matrix4 Transform;
	};

	class ATHENA_API DrawListStatic
	{
	public:
		void Push(const StaticDrawCall& drawCall);
		void Sort();

		void Flush(const Ref<RenderCommandBuffer> commandBuffer, const Ref<Pipeline>& pipeline);
		void FlushShadowPass(const Ref<RenderCommandBuffer> commandBuffer, const Ref<Pipeline>& pipeline);

		void SetInstanceOffset(uint32 offset) { m_InstanceOffset = offset; }
		uint32 GetInstancesCount() const;

		uint64 Size() const { return m_Array.size(); }
		void Clear();

		const auto& GetArray() const { return m_Array; }

	private:
		std::vector<StaticDrawCall> m_Array;
		uint32 m_InstanceOffset = 0;
	};

	struct AnimDrawCall
	{
		Ref<VertexBuffer> VertexBuffer;
		Ref<Material> Material;
		Matrix4 Transform;
		uint32 BonesOffset = 0;
	};

	class ATHENA_API DrawListAnim
	{
	public:
		void Push(const AnimDrawCall& drawCall);
		void Sort();

		void Flush(const Ref<RenderCommandBuffer> commandBuffer, const Ref<Pipeline>& pipeline);
		void FlushShadowPass(const Ref<RenderCommandBuffer> commandBuffer, const Ref<Pipeline>& pipeline);

		void SetInstanceOffset(uint32 offset) { m_InstanceOffset = offset; }

		uint64 Size() const { return m_Array.size(); }
		void Clear();

		const auto& GetArray() const { return m_Array; }

	private:
		std::vector<AnimDrawCall> m_Array;
		uint32 m_InstanceOffset = 0;
	};
}
