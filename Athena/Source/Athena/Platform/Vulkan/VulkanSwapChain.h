#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/SwapChain.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	class VulkanSwapChain : public SwapChain
	{
	public:
		VulkanSwapChain(void* windowHandle, bool vsync = false);
		~VulkanSwapChain();

		void CleanUp(VkSwapchainKHR swapChain, const std::vector<VkImageView>& imageViews, bool cleanupSurface = false);

		virtual bool Recreate() override;
		virtual void SetVSync(bool enabled) override;

		virtual void AcquireImage() override;
		virtual void Present() override;

		virtual void* GetCurrentImageHandle() override;
		virtual uint32 GetCurrentImageIndex() override { return m_ImageIndex; }

		const std::vector<VkImageView> GetVulkanImageViews() const { return m_SwapChainImageViews; };

	private:
		VkSurfaceKHR m_Surface;
		VkSwapchainKHR m_VkSwapChain;

		std::vector<VkImage> m_SwapChainImages;
		std::vector<VkImageView> m_SwapChainImageViews;

		VkPresentModeKHR m_SelectedPresentMode;
		VkSurfaceFormatKHR m_SelectedFormat;

		uint32 m_ImageIndex;
		bool m_VSync;
		bool m_Dirty;
	};
}
