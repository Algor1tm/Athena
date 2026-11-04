#include "VulkanSwapChain.h"

#include "Athena/Core/Stats.h"
#include "Athena/Core/Application.h"

#include "Athena/Renderer/RHI/Vulkan/VulkanRHI.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanUtils.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


namespace Athena
{
	EXTERN_CYCLE_STAT(STAT_WaitForGPU);

	VulkanSwapChain::VulkanSwapChain(void* windowHandle, bool vsync)
		: m_WindowHandle(windowHandle), 
		m_VSync(vsync),
		m_Dirty(true),
		m_ImageIndex(0),
		m_Width(0),
		m_Height(0)
	{

	}

	VulkanSwapChain::~VulkanSwapChain()
	{
		VulkanRHI::Get()->DeleteFrameResourcesDeferred([contexts = m_FrameContexts]()
			{
				for (uint32 i = 0; i < Vulkan::GetMaxFramesInFlight(); ++i)
				{
					vkDestroySemaphore(Vulkan::GetLogicalDevice(), contexts[i].RenderingCompletedSemaphore, nullptr);
					vkDestroySemaphore(Vulkan::GetLogicalDevice(), contexts[i].ImageAcquiredSemaphore, nullptr);

					contexts[i].CommandBuffer->WaitCompletion();
					delete contexts[i].CommandBuffer;
				}
			});

		CleanUp(m_VkSwapChain, true);
	}

	void VulkanSwapChain::Init()
	{
		ensuref(glfwVulkanSupported());

		// Present on graphics context
		m_PresentContext = VulkanRHI::Get()->GetDevice()->GetGraphicsContext();

		VkPhysicalDevice physicalDevice = VulkanRHI::Get()->GetDevice()->GetPhysicalDevice();

		// Create window surface
		{
			m_Surface = VK_NULL_HANDLE;
			VK_CHECK(glfwCreateWindowSurface(VulkanRHI::Get()->GetVulkanInstance(), (GLFWwindow*)m_WindowHandle, nullptr, &m_Surface));
			ATN_LOG_INFO(Vulkan, "Create Window Surface");

			VkBool32 supportWSI;
			vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, m_PresentContext->GetQueueFamily(), m_Surface, &supportWSI);
			ensure(supportWSI, "Selected Queue Family does not support WSI!");
		}

		// Query device properties and create SwapChain
		{
			m_VkSwapChain = VK_NULL_HANDLE;

			VkSurfaceCapabilitiesKHR surfaceCaps;
			std::vector<VkSurfaceFormatKHR> surfaceFormats;
			std::vector<VkPresentModeKHR> surfacePresentModes;

			// Surface Capabilites
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, m_Surface, &surfaceCaps);

			int32 MaxFramesInFlight = ConsoleManager::Get().FindCVar("r.MaxFramesInFlight")->GetInt();

			ensure(MaxFramesInFlight >= surfaceCaps.minImageCount && MaxFramesInFlight <= surfaceCaps.maxImageCount,
				"Current vulkan device does not support requested r.MaxFramesInFlight value - {}, min - {}, max - {}", surfaceCaps.minImageCount, surfaceCaps.maxImageCount);

			// Format
			{
				// Supported formats
				uint32 formatCount;
				vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_Surface, &formatCount, nullptr);

				surfaceFormats.resize(formatCount);
				vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_Surface, &formatCount, surfaceFormats.data());

				// Select Format
				const VkFormat requestedFormats[] = { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM };
				const VkColorSpaceKHR requestedColorsSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

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

				m_Format = surfaceFormats[0];
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

				m_PresentMode = surfacePresentModes[0];
			}

			for (uint32 i = 0; i < Vulkan::GetMaxFramesInFlight(); ++i)
			{
				VkSemaphoreCreateInfo semaphoreCI = {};
				semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

				VK_CHECK(vkCreateSemaphore(Vulkan::GetLogicalDevice(), &semaphoreCI, nullptr, &m_FrameContexts[i].ImageAcquiredSemaphore));
				VK_SET_OBJECT_NAME(m_FrameContexts[i].ImageAcquiredSemaphore, VK_OBJECT_TYPE_SEMAPHORE, std::format("ImageAcquireSemaphore_{}", i));

				VK_CHECK(vkCreateSemaphore(Vulkan::GetLogicalDevice(), &semaphoreCI, nullptr, &m_FrameContexts[i].RenderingCompletedSemaphore));
				VK_SET_OBJECT_NAME(m_FrameContexts[i].RenderingCompletedSemaphore, VK_OBJECT_TYPE_SEMAPHORE, std::format("RenderingCompletedSemaphore_{}", i));

				m_FrameContexts[i].CommandBuffer = new VulkanCommandBuffer(m_PresentContext.Raw());
			}

			Recreate();
		}
	}

	void VulkanSwapChain::CleanUp(VkSwapchainKHR swapChain, bool cleanupSurface)
	{
		FramesInFlightArray<VkImageView> imageViews;

		for (uint32 i = 0; i < Vulkan::GetMaxFramesInFlight(); ++i)
			imageViews[i] = m_FrameContexts[i].SwapChainImageView;

		VulkanRHI::Get()->DeleteFrameResourcesDeferred([swapChain = swapChain, views = imageViews, surface = m_Surface, cleanupSurface = cleanupSurface]()
		{
			for (uint32 i = 0; i < Vulkan::GetMaxFramesInFlight(); ++i)
				vkDestroyImageView(Vulkan::GetLogicalDevice(), views[i], nullptr);

			vkDestroySwapchainKHR(Vulkan::GetLogicalDevice(), swapChain, nullptr);

			if (cleanupSurface)
				vkDestroySurfaceKHR(VulkanRHI::Get()->GetVulkanInstance(), surface, nullptr);
		});
	}

	bool VulkanSwapChain::Recreate()
	{
		TRACY_PROFILE_FUNC();

		vkDeviceWaitIdle(Vulkan::GetLogicalDevice());

		auto& app = Application::Get();

		uint32 framesInFlight = Vulkan::GetMaxFramesInFlight();
		VkSwapchainKHR oldSwapChain = m_VkSwapChain;

		VkSurfaceCapabilitiesKHR surfaceCaps;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VulkanRHI::Get()->GetDevice()->GetPhysicalDevice(), m_Surface, &surfaceCaps);

		if (surfaceCaps.currentExtent.width == (uint32)-1)
		{
			m_Width = app.GetWindow().GetWidth();
			m_Height = app.GetWindow().GetHeight();

			m_Width = Math::Clamp(m_Width, surfaceCaps.minImageExtent.width, surfaceCaps.maxImageExtent.width);
			m_Height = Math::Clamp(m_Height, surfaceCaps.minImageExtent.height, surfaceCaps.maxImageExtent.height);
		}
		else
		{
			m_Width = surfaceCaps.currentExtent.width;
			m_Height = surfaceCaps.currentExtent.height;
		}

		if (m_Width == 0 || m_Height == 0)
			return false;

		VkSwapchainCreateInfoKHR swapchainCI = {};
		swapchainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchainCI.surface = m_Surface;
		swapchainCI.minImageCount = framesInFlight;
		swapchainCI.imageFormat = m_Format.format;
		swapchainCI.imageColorSpace = m_Format.colorSpace;
		swapchainCI.imageExtent.width = m_Width;
		swapchainCI.imageExtent.height = m_Height;
		swapchainCI.imageArrayLayers = 1;
		swapchainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		swapchainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;           // Assume that graphics family == present family
		swapchainCI.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		swapchainCI.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapchainCI.presentMode = m_VSync ? VK_PRESENT_MODE_FIFO_KHR : m_PresentMode;
		swapchainCI.clipped = VK_TRUE;
		swapchainCI.oldSwapchain = oldSwapChain;

		VK_CHECK(vkCreateSwapchainKHR(Vulkan::GetLogicalDevice(), &swapchainCI, nullptr, &m_VkSwapChain));

		if (oldSwapChain != VK_NULL_HANDLE)
		{
			CleanUp(oldSwapChain, false);
		}

		uint32 imagesCount;
		vkGetSwapchainImagesKHR(Vulkan::GetLogicalDevice(), m_VkSwapChain, &imagesCount, nullptr);
		ensuref(imagesCount == framesInFlight);

		std::vector<VkImage> swapChainImages(Vulkan::GetMaxFramesInFlight());
		VK_CHECK(vkGetSwapchainImagesKHR(Vulkan::GetLogicalDevice(), m_VkSwapChain, &imagesCount, swapChainImages.data()));

		std::vector<VkImageView> swapChainImageViews(Vulkan::GetMaxFramesInFlight());
		for (uint32_t i = 0; i < framesInFlight; i++)
		{
			VkImageViewCreateInfo imageViewCI = {};
			imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
			imageViewCI.image = swapChainImages[i];
			imageViewCI.format = m_Format.format;
			imageViewCI.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewCI.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewCI.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewCI.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageViewCI.subresourceRange.baseMipLevel = 0;
			imageViewCI.subresourceRange.levelCount = 1;
			imageViewCI.subresourceRange.baseArrayLayer = 0;
			imageViewCI.subresourceRange.layerCount = 1;

			VK_CHECK(vkCreateImageView(Vulkan::GetLogicalDevice(), &imageViewCI, nullptr, &swapChainImageViews[i]));

			VK_SET_OBJECT_NAME(swapChainImages[i], VK_OBJECT_TYPE_IMAGE, std::format("SwapChainImage_{}", i));
			VK_SET_OBJECT_NAME(swapChainImageViews[i], VK_OBJECT_TYPE_IMAGE_VIEW, std::format("SwapChainImageView_{}", i));

			m_FrameContexts[i].SwapChainImage = swapChainImages[i];
			m_FrameContexts[i].SwapChainImageView = swapChainImageViews[i];
		}

		// TODO: remove ui layer logic from here
		auto uiLayer = Application::Get().GetImGuiLayer();
		if (uiLayer)
			uiLayer->OnSwapChainRecreate();

		m_Dirty = false;
		return true;
	}

	void VulkanSwapChain::SetVSync(bool enabled)
	{
		if (m_VSync == enabled)
			return;

		m_Dirty = true;
		m_VSync = enabled;
	}

	void VulkanSwapChain::OnWindowResized(uint32 width, uint32 height)
	{
		m_Dirty = true;
	}

	void VulkanSwapChain::AcquireImage()
	{
		if (m_Dirty)
			Recreate();

		FrameContext& currentFrameContext = m_FrameContexts[VulkanRHI::Get()->GetFrameResourcesIndex()];

		{
			TRACY_PROFILE_SCOPE("WaitForGPU");
			SCOPE_CYCLE_STAT(STAT_WaitForGPU);
		
			currentFrameContext.CommandBuffer->WaitCompletion();
		}
		
		{
			TRACY_PROFILE_SCOPE("AcquireNextImage");

			VkSemaphore imageAcquiredSemaphore = currentFrameContext.ImageAcquiredSemaphore;
			VkResult result = vkAcquireNextImageKHR(Vulkan::GetLogicalDevice(), m_VkSwapChain, UINT64_MAX, imageAcquiredSemaphore, VK_NULL_HANDLE, &m_ImageIndex);
		
			if (result == VK_ERROR_OUT_OF_DATE_KHR)
			{
				Recreate();
				VK_CHECK(vkAcquireNextImageKHR(Vulkan::GetLogicalDevice(), m_VkSwapChain, UINT64_MAX, imageAcquiredSemaphore, VK_NULL_HANDLE, &m_ImageIndex));
			}
			else if (result == VK_SUBOPTIMAL_KHR)
			{
				m_Dirty = true;
			}
			else
			{
				VK_CHECK(result);
			}
		}
	}

	void VulkanSwapChain::Present()
	{
		TRACY_PROFILE_FUNC();

		VkSemaphore waitSemaphore = m_FrameContexts[m_ImageIndex].RenderingCompletedSemaphore;
		
		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &waitSemaphore;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &m_VkSwapChain;
		presentInfo.pImageIndices = &m_ImageIndex;
		
		VkResult result = vkQueuePresentKHR(m_PresentContext->GetQueue(), &presentInfo);
		
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		{
			m_Dirty = true;
		}
		else
		{
			VK_CHECK(result);
		}
	}

	VulkanCommandBuffer* VulkanSwapChain::BeginRendering()
	{
		VulkanCommandBuffer* commandBuffer = m_FrameContexts[VulkanRHI::Get()->GetFrameResourcesIndex()].CommandBuffer;

		commandBuffer->Begin();
		return commandBuffer;
	}

	void VulkanSwapChain::EndRendering(VkPipelineStageFlags2 imageAcquireWaitFlags)
	{
		FrameContext* frameContext = &m_FrameContexts[VulkanRHI::Get()->GetFrameResourcesIndex()];

		VulkanCommandBuffer* commandBuffer = frameContext->CommandBuffer;
		commandBuffer->AddDependency(frameContext->ImageAcquiredSemaphore, 0, imageAcquireWaitFlags);

		VkSemaphoreSubmitInfo semaphoreSignalInfo = {};
		semaphoreSignalInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		semaphoreSignalInfo.semaphore = m_FrameContexts[m_ImageIndex].RenderingCompletedSemaphore;
		semaphoreSignalInfo.value = 0;
		semaphoreSignalInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		semaphoreSignalInfo.deviceIndex = 0;

		commandBuffer->End();
		commandBuffer->Submit(m_PresentContext->GetQueue(), &semaphoreSignalInfo);
	}

	VkImage VulkanSwapChain::GetCurrentVulkanImage() const
	{
		return m_FrameContexts[m_ImageIndex].SwapChainImage;
	}

	VkImageView VulkanSwapChain::GetVulkanImageView(uint32 index) const
	{
		return m_FrameContexts[index].SwapChainImageView;
	}
}
