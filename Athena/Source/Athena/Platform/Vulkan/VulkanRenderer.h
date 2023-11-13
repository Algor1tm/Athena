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

		virtual void BeginFrame() override;
		virtual void EndFrame() override;

		virtual void RenderMeshWithMaterial(const Ref<VertexBuffer>& mesh, const Ref<Material>& material) override;

		virtual void BlitToScreen(const Ref<Texture2D>& texture) override;
		virtual void WaitDeviceIdle() override;

		virtual void GetRenderCapabilities(RenderCapabilities& caps) override;
		virtual uint64 GetMemoryUsage() override;

	private:
		std::vector<VkCommandBuffer> m_VkCommandBuffers;
	};
}
