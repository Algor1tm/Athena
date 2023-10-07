#include "VulkanSwapChain.h"

#include "Athena/Core/Application.h"
#include "Athena/Renderer/Renderer.h"

#include "Athena/Platform/Vulkan/VulkanDebug.h"
#include "Athena/Platform/Vulkan/VulkanRenderer.h"

#include <GLFW/glfw3.h>


namespace Athena
{
	VulkanSwapChain::VulkanSwapChain(void* windowHandle, bool vsync)
	{
		m_VSync = vsync;

		VkPhysicalDevice physicalDevice = VulkanContext::GetCurrentDevice()->GetPhysicalDevice();
		uint32 queueFamilyIndex = VulkanContext::GetCurrentDevice()->GetQueueFamily();

		// Create window surface
		{
			m_Surface = VK_NULL_HANDLE;
			VK_CHECK(glfwCreateWindowSurface(VulkanContext::GetInstance(), (GLFWwindow*)windowHandle, VulkanContext::GetAllocator(), &m_Surface));
			ATN_CORE_INFO_TAG("Vulkan", "Create Window Surface");

			VkBool32 supportWSI;
			vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamilyIndex, m_Surface, &supportWSI);
			ATN_CORE_VERIFY(supportWSI, "Selected Queue Family does not support WSI!");
		}

		// Query device properties and create SwapChain
		{
			m_VkSwapChain = VK_NULL_HANDLE;

			VkSurfaceCapabilitiesKHR surfaceCaps;
			std::vector<VkSurfaceFormatKHR> surfaceFormats;
			std::vector<VkPresentModeKHR> surfacePresentModes;

			// Surface Capabilites
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, m_Surface, &surfaceCaps);
			ATN_CORE_VERIFY(surfaceCaps.minImageCount <= Renderer::FramesInFlight() && surfaceCaps.maxImageCount >= Renderer::FramesInFlight());

			// Format
			{
				// Supported formats
				uint32 formatCount;
				vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_Surface, &formatCount, nullptr);

				surfaceFormats.resize(formatCount);
				vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_Surface, &formatCount, surfaceFormats.data());

				// Select Format
				const VkFormat requestedFormats[] = { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM };
				const VkColorSpaceKHR requestedColorsSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;

				// first format in the array is the most prefered
				const auto rateFormat = [requestedFormats, requestedColorsSpace](VkSurfaceFormatKHR fmt) -> uint64
				{
					if (fmt.colorSpace != requestedColorsSpace)
						return 0;

					auto findPtr = std::find(std::begin(requestedFormats), std::end(requestedFormats), fmt.format);
					uint64 reversedIdx = std::distance(findPtr, std::end(requestedFormats)); // if last element - index is 1

					return reversedIdx;
				};

				std::sort(surfaceFormats.begin(), surfaceFormats.end(), [rateFormat, requestedFormats, requestedColorsSpace](VkSurfaceFormatKHR left, VkSurfaceFormatKHR right)
					{
						return rateFormat(left) > rateFormat(right);
					});

				m_SelectedFormat = surfaceFormats[0];
			}

			// Present mode
			{
				// Supported present modes
				uint32 presentModeCount;
				vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_Surface, &presentModeCount, nullptr);

				surfacePresentModes.resize(presentModeCount);
				vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_Surface, &presentModeCount, surfacePresentModes.data());

				const VkPresentModeKHR requestedPresentModes[] =
				{ VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_FIFO_RELAXED_KHR, VK_PRESENT_MODE_FIFO_KHR };

				// first mode in the array is the most prefered
				const auto ratePresentMode = [requestedPresentModes](VkPresentModeKHR mode) -> uint64
				{
					auto findPtr = std::find(std::begin(requestedPresentModes), std::end(requestedPresentModes), mode);
					uint64 reversedIdx = std::distance(findPtr, std::end(requestedPresentModes));

					return reversedIdx;
				};

				std::sort(surfacePresentModes.begin(), surfacePresentModes.end(), [ratePresentMode, requestedPresentModes](VkPresentModeKHR left, VkPresentModeKHR right)
					{
						return ratePresentMode(left) > ratePresentMode(right);
					});

				m_SelectedPresentMode = surfacePresentModes[0];
			}


			Recreate();
		}
	}

	VulkanSwapChain::~VulkanSwapChain()
	{
		CleanUp(m_VkSwapChain, m_SwapChainImageViews);
		vkDestroySurfaceKHR(VulkanContext::GetInstance(), m_Surface, VulkanContext::GetAllocator());
	}

	void VulkanSwapChain::CleanUp(VkSwapchainKHR swapChain, const std::vector<VkImageView>& imageViews)
	{
		for (uint32 i = 0; i < Renderer::FramesInFlight(); ++i)
			vkDestroyImageView(VulkanContext::GetCurrentDevice()->GetLogicalDevice(), imageViews[i], VulkanContext::GetAllocator());

		vkDestroySwapchainKHR(VulkanContext::GetCurrentDevice()->GetLogicalDevice(), swapChain, VulkanContext::GetAllocator());
	}

	void VulkanSwapChain::Recreate()
	{
		VkPhysicalDevice physicalDevice = VulkanContext::GetCurrentDevice()->GetPhysicalDevice();
		VkDevice logicalDevice = VulkanContext::GetCurrentDevice()->GetLogicalDevice();
		uint32 framesInFlight = Renderer::FramesInFlight();
		VkSwapchainKHR oldSwapChain = m_VkSwapChain;

		VkSurfaceCapabilitiesKHR surfaceCaps;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, m_Surface, &surfaceCaps);

		VkExtent2D extent;
		if (surfaceCaps.currentExtent.width == (uint32)-1)
		{
			extent.width = Application::Get().GetWindow().GetWidth();
			extent.height = Application::Get().GetWindow().GetHeight();

			extent.width = Math::Clamp(extent.width, surfaceCaps.minImageExtent.width, surfaceCaps.maxImageExtent.width);
			extent.height = Math::Clamp(extent.width, surfaceCaps.minImageExtent.height, surfaceCaps.maxImageExtent.height);
		}
		else
		{
			extent.width = surfaceCaps.currentExtent.width;
			extent.height = surfaceCaps.currentExtent.height;
		}

		VkSwapchainCreateInfoKHR swapchainCI = {};
		swapchainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchainCI.surface = m_Surface;
		swapchainCI.minImageCount = framesInFlight;
		swapchainCI.imageFormat = m_SelectedFormat.format;
		swapchainCI.imageColorSpace = m_SelectedFormat.colorSpace;
		swapchainCI.imageExtent.width = extent.width;
		swapchainCI.imageExtent.height = extent.height;
		swapchainCI.imageArrayLayers = 1;
		swapchainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		swapchainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;           // Assume that graphics family == present family
		swapchainCI.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		swapchainCI.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapchainCI.presentMode = GetPresentMode();
		swapchainCI.clipped = VK_TRUE;
		swapchainCI.oldSwapchain = oldSwapChain;

		vkDeviceWaitIdle(logicalDevice);

		VK_CHECK(vkCreateSwapchainKHR(logicalDevice, &swapchainCI, VulkanContext::GetAllocator(), &m_VkSwapChain));

		if (oldSwapChain != VK_NULL_HANDLE)
			CleanUp(oldSwapChain, m_SwapChainImageViews);

		uint32 imagesCount;
		vkGetSwapchainImagesKHR(logicalDevice, m_VkSwapChain, &imagesCount, nullptr);
		ATN_CORE_VERIFY(imagesCount == framesInFlight);

		m_SwapChainImages.resize(framesInFlight);
		VK_CHECK(vkGetSwapchainImagesKHR(logicalDevice, m_VkSwapChain, &imagesCount, m_SwapChainImages.data()));

		m_SwapChainImageViews.resize(framesInFlight);
		for (uint32_t i = 0; i < framesInFlight; i++)
		{
			VkImageViewCreateInfo imageViewCI = {};
			imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			imageViewCI.format = m_SelectedFormat.format;
			imageViewCI.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewCI.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewCI.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewCI.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageViewCI.subresourceRange.baseMipLevel = 0;
			imageViewCI.subresourceRange.levelCount = 1;
			imageViewCI.subresourceRange.baseArrayLayer = 0;
			imageViewCI.subresourceRange.layerCount = 1;
			imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
			imageViewCI.image = m_SwapChainImages[i];

			VK_CHECK(vkCreateImageView(logicalDevice, &imageViewCI, VulkanContext::GetAllocator(), &m_SwapChainImageViews[i]));
		}

		m_Dirty = false;
	}

	void VulkanSwapChain::SetVSync(bool enabled)
	{
		if (m_VSync == enabled)
			return;

		m_Dirty = true;
		m_VSync = enabled;
	}

	VkPresentModeKHR VulkanSwapChain::GetPresentMode()
	{
		return m_VSync ? VK_PRESENT_MODE_FIFO_KHR : m_SelectedPresentMode;
	}

	void VulkanSwapChain::AcquireImage()
	{
		if (m_Dirty)
		{
			Recreate();
		}

		// We save frame index, because when we get to presenting it will change
		m_CurrentFrameIndex = Renderer::CurrentFrameIndex();

		VkDevice logicalDevice = VulkanContext::GetCurrentDevice()->GetLogicalDevice();
		const FrameSyncData& frameData = VulkanContext::GetFrameSyncData(m_CurrentFrameIndex);

		vkWaitForFences(logicalDevice, 1, &frameData.RenderCompleteFence, VK_TRUE, UINT64_MAX);

		m_ImageIndex = 0;
		VkResult result = vkAcquireNextImageKHR(logicalDevice, m_VkSwapChain, UINT64_MAX, frameData.ImageAcquiredSemaphore, VK_NULL_HANDLE, &m_ImageIndex);
		
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) 
		{
			m_Dirty = true;
		}
		else
		{
			VK_CHECK(result);
		}

		vkResetFences(logicalDevice, 1, &frameData.RenderCompleteFence);
	}

	void VulkanSwapChain::Present()
	{
		if (m_Dirty)
			return;

		const FrameSyncData& frameData = VulkanContext::GetFrameSyncData(m_CurrentFrameIndex);

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &frameData.RenderCompleteSemaphore;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &m_VkSwapChain;
		presentInfo.pImageIndices = &m_ImageIndex;

		VkResult result = vkQueuePresentKHR(VulkanContext::GetCurrentDevice()->GetQueue(), &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		{
			m_Dirty = true;
		}
		else
		{
			VK_CHECK(result);
		}
	}

	void* VulkanSwapChain::GetCurrentImageHandle()
	{
		return (void*)m_SwapChainImages[m_ImageIndex];
	}
}
