#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/RendererAPI.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	class VulkanRenderer
	{
	public:
		virtual void Init() = 0;

		static VkInstance GetInstance() { return s_Instance; }


		virtual void BindPipeline(const Pipeline& pipeline) {};

		virtual void BeginRenderPass(const RenderPass& pass) {};
		virtual void EndRenderPass() = 0;

		virtual void BeginComputePass(const ComputePass& pass) {};
		virtual void EndComputePass() {};

		virtual void DrawTriangles(const Ref<VertexBuffer>& vertexBuffer, uint32 indexCount = 0) {};
		virtual void DrawLines(const Ref<VertexBuffer>& vertexBuffer, uint32 vertexCount = 0) {};

		virtual void Dispatch(uint32 x, uint32 y, uint32 z, Vector3i workGroupSize) {};



	private:
		static VkInstance s_Instance;
	};
}
