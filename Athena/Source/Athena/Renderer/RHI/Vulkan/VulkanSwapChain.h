#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/SwapChain.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanRHI.h"


namespace Athena
{
	class VulkanSwapChain : public SwapChain
	{
	public:
		VulkanSwapChain(void* windowHandle, bool vsync = false);
		virtual ~VulkanSwapChain();

		virtual void Init() override;

		virtual void OnWindowResized(uint32 width, uint32 height) override;
		virtual void SetVSync(bool enabled) override;

		void CleanUp(VkSwapchainKHR swapChain, bool cleanupSurface = false);

		void AcquireImage();
		void Present();

		uint32 GetCurrentImageIndex() const { return m_ImageIndex; }
		VkImage GetCurrentVulkanImage() const;

		uint32 GetWidth() const { return m_Width; }
		uint32 GetHeight() const { return m_Height; }

		VkSemaphore GetImageAcquiredSemaphore() const;

		const std::vector<VkImageView>& GetVulkanImageViews() const { return m_SwapChainImageViews; }
		VkFormat GetFormat() const { return m_Format.format; }

	private:
		bool Recreate();

	private:
		VkSurfaceKHR m_Surface;
		VkSwapchainKHR m_VkSwapChain;

		std::vector<VkImage> m_SwapChainImages;
		std::vector<VkImageView> m_SwapChainImageViews;
		std::vector<VkSemaphore> m_ImageAcquiredSemaphores;

		VkPresentModeKHR m_PresentMode;
		VkSurfaceFormatKHR m_Format;
		uint32 m_Width;
		uint32 m_Height;

		uint32 m_ImageIndex;
		bool m_VSync;
		bool m_Dirty;
		void* m_WindowHandle;
	};
}
