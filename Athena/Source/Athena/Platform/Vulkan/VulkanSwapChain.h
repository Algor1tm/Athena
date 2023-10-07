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

		void CleanUp(VkSwapchainKHR swapChain, const std::vector<VkImageView>& imageViews);

		virtual void Recreate() override;
		virtual void SetVSync(bool enabled) override;

		virtual void AcquireImage() override;
		virtual void Present() override;

		virtual void* GetCurrentImageHandle() override;

	private:
		VkPresentModeKHR GetPresentMode();

	private:
		VkSurfaceKHR m_Surface;
		VkSwapchainKHR m_VkSwapChain;

		std::vector<VkImage> m_SwapChainImages;
		std::vector<VkImageView> m_SwapChainImageViews;

		uint32 m_ImageIndex = 0;
		uint32 m_CurrentFrameIndex = 0;

		bool m_VSync = false;
		VkPresentModeKHR m_SelectedPresentMode;
		VkSurfaceFormatKHR m_SelectedFormat;
		bool m_Dirty = false;
	};
}
