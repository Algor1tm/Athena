#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/SwapChain.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	class VulkanSwapChain : public SwapChain
	{
	public:
		VulkanSwapChain(void* windowHandle);
		~VulkanSwapChain();

		virtual void AcquireImage() override;
		virtual void Present() override;

		virtual void* GetCurrentImageHandle() override;

	private:
		VkSurfaceKHR m_Surface;
		VkSwapchainKHR m_VkSwapChain;

		std::vector<VkImage> m_SwapChainImages;
		std::vector<VkImageView> m_SwapChainImageViews;

		uint32 m_ImageIndex = 0;
		uint32 m_CurrentFrameIndex = 0;
	};
}
