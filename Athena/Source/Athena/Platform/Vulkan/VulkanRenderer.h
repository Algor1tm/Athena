#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/RendererAPI.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	class VulkanRenderer: public RendererAPI
	{
	public:
		virtual void Init() override;
		virtual void Shutdown() override;

		virtual void OnUpdate() override;

		virtual void RenderGeometry(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<Pipeline>& pipeline, const Ref<VertexBuffer>& vertexBuffer, const Ref<Material>& material) override;
		virtual void Dispatch(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<ComputePipeline>& pipeline, Vector3i imageSize, const Ref<Material>& material) override;

		virtual void BeginDebugRegion(const Ref<RenderCommandBuffer>& cmdBuffer, std::string_view name, const Vector4& color) override;
		virtual void EndDebugRegion(const Ref<RenderCommandBuffer>& cmdBuffer) override;
		virtual void InsertDebugMarker(const Ref<RenderCommandBuffer>& cmdBuffer, std::string_view name, const Vector4& color) override;

		virtual void BlitToScreen(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<Image>& image) override;
		virtual void WaitDeviceIdle() override;

		virtual void GetRenderCapabilities(RenderCapabilities& caps) override;
		virtual uint64 GetMemoryUsage() override;

	private:
		std::vector<VkCommandBuffer> m_VkCommandBuffers;
		PFN_vkCmdDebugMarkerBeginEXT m_DebugMarkBeginPFN;
		PFN_vkCmdDebugMarkerEndEXT m_DebugMarkEndPFN;
		PFN_vkCmdDebugMarkerInsertEXT m_DebugMarkInsertPFN;
	};
}
