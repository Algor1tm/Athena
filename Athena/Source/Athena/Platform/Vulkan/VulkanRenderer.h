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

		virtual void WaitDeviceIdle() override;
		virtual void CopyTextureToSwapChain(const Ref<Texture2D>& texture) override;

		virtual RenderCapabilities GetRenderCapabilities() override;

	private:
		bool CheckEnabledExtensions(const std::vector<const char*>& requiredExtensions);
		bool CheckEnabledLayers(const std::vector<const char*>& requiredLayers);

	private:
		std::vector<VkCommandBuffer> m_VkCommandBuffers;
		VkDebugReportCallbackEXT m_DebugReport;
	};
}
