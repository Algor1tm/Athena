#include "VulkanRenderer.h"

#include "Athena/Core/Application.h"

#include "Athena/Platform/Vulkan/VulkanDevice.h"
#include "Athena/Platform/Vulkan/VulkanSwapChain.h"
#include "Athena/Platform/Vulkan/VulkanUtils.h"
#include "Athena/Platform/Vulkan/VulkanTexture2D.h"


namespace Athena
{
	VkInstance VulkanContext::s_Instance = VK_NULL_HANDLE;
	VkAllocationCallbacks* VulkanContext::s_Allocator = VK_NULL_HANDLE;
	Ref<VulkanDevice> VulkanContext::s_CurrentDevice = VK_NULL_HANDLE;
	std::vector<FrameSyncData> VulkanContext::s_FrameSyncData;
	VkCommandPool VulkanContext::s_CommandPool = VK_NULL_HANDLE;
	VkCommandBuffer VulkanContext::s_ActiveCommandBuffer = VK_NULL_HANDLE;

#ifdef ATN_DEBUG
	inline VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType,
		uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
	{
		switch (flags)
		{
		case VK_DEBUG_REPORT_INFORMATION_BIT_EXT:
			ATN_CORE_INFO_TAG("Vulkan", "Debug report: \n{}\n", pMessage); break;

		case VK_DEBUG_REPORT_WARNING_BIT_EXT:
			ATN_CORE_WARN_TAG("Vulkan", "Debug report: \n{}\n", pMessage); break;

		case VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT:
			ATN_CORE_WARN_TAG("Vulkan", "'PERFORMANCE WARNING' Debug report: \n{}\n", pMessage); break;

		case VK_DEBUG_REPORT_ERROR_BIT_EXT:
			ATN_CORE_ERROR_TAG("Vulkan", "Debug report: \n{}\n", pMessage); break;
		}

		return VK_FALSE;
	}
#endif

	void VulkanRenderer::Init()
	{
		ATN_CORE_VERIFY(VulkanContext::s_Instance == VK_NULL_HANDLE, "Vulkan Instance already exists!");

		// TODO: Add custom allocator
		VulkanContext::s_Allocator = VK_NULL_HANDLE;

		Renderer::Submit([this]()
		{
			// Create Vulkan Instance
			{
				// Select Vulkan Version
				uint32 version = 0;
				VK_CHECK(vkEnumerateInstanceVersion(&version));

				uint32 variant = VK_API_VERSION_VARIANT(version);
				uint32 major = VK_API_VERSION_MAJOR(version);
				uint32 minor = VK_API_VERSION_MINOR(version);
				uint32 patch = VK_API_VERSION_PATCH(version);

				ATN_CORE_INFO_TAG("Vulkan", "Version: {}.{}.{}.{}", variant, major, minor, patch);

				VkApplicationInfo appInfo = {};
				appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
				appInfo.pNext = nullptr;
				appInfo.pApplicationName = Application::Get().GetConfig().Name.c_str();
				appInfo.pEngineName = "Athena";
				appInfo.apiVersion = version;

				// Select Extensions
				// Note: Vulkan initializes before GLFW, cant call glfwGetRequiredInstanceExtensions
				std::vector<const char*> extensions = {
					"VK_KHR_surface",
					"VK_KHR_win32_surface" };

				std::vector<const char*> layers;

	#ifdef ATN_DEBUG
				layers.push_back("VK_LAYER_KHRONOS_validation");
				extensions.push_back("VK_EXT_debug_report");
	#endif

				String message = "Vulkan Extensions: \n\t";
				for (auto ext : extensions)
					message.append(std::format("'{}'\n\t", ext));

				ATN_CORE_INFO_TAG("Vulkan", message);

				// Create Vulkan Instance
				VkInstanceCreateInfo instanceCI = {};
				instanceCI.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
				instanceCI.pNext = nullptr;
				instanceCI.flags = 0;
				instanceCI.pApplicationInfo = &appInfo;
				instanceCI.enabledLayerCount = layers.size();
				instanceCI.ppEnabledLayerNames = layers.data();
				instanceCI.enabledExtensionCount = extensions.size();
				instanceCI.ppEnabledExtensionNames = extensions.data();

				VK_CHECK(vkCreateInstance(&instanceCI, VulkanContext::s_Allocator, &VulkanContext::s_Instance));

	#ifdef ATN_DEBUG
				// Setup the debug report callback
				auto vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(VulkanContext::GetInstance(), "vkCreateDebugReportCallbackEXT");
				ATN_CORE_ASSERT(vkCreateDebugReportCallbackEXT != NULL);

				VkDebugReportCallbackCreateInfoEXT reportCI = {};
				reportCI.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
				reportCI.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
				reportCI.pfnCallback = VulkanDebugCallback;
				reportCI.pUserData = NULL;
				VK_CHECK(vkCreateDebugReportCallbackEXT(VulkanContext::GetInstance(), &reportCI, VulkanContext::GetAllocator(), &m_DebugReport));
	#endif
			}
		});


		// Create Device
		{
			VulkanContext::s_CurrentDevice = Ref<VulkanDevice>::Create();
		}

		Renderer::Submit([this]()
		{
			// Create synchronization primitives
			{
				VulkanContext::s_FrameSyncData.resize(Renderer::GetFramesInFlight());

				for (uint32_t i = 0; i < Renderer::GetFramesInFlight(); i++)
				{
					VkSemaphoreCreateInfo semaphoreCI = {};
					semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
					VK_CHECK(vkCreateSemaphore(VulkanContext::GetLogicalDevice(), &semaphoreCI, VulkanContext::GetAllocator(), &VulkanContext::s_FrameSyncData[i].ImageAcquiredSemaphore));
					VK_CHECK(vkCreateSemaphore(VulkanContext::GetLogicalDevice(), &semaphoreCI, VulkanContext::GetAllocator(), &VulkanContext::s_FrameSyncData[i].RenderCompleteSemaphore));

					VkFenceCreateInfo fenceCI = {};
					fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
					fenceCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;
					VK_CHECK(vkCreateFence(VulkanContext::GetLogicalDevice(), &fenceCI, VulkanContext::GetAllocator(), &VulkanContext::s_FrameSyncData[i].RenderCompleteFence));

				}
			}

			// Create CommandPool
			{
				VkCommandPoolCreateInfo commandPoolCI = {};
				commandPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
				commandPoolCI.queueFamilyIndex = VulkanContext::GetDevice()->GetQueueFamily();
				commandPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
				VK_CHECK(vkCreateCommandPool(VulkanContext::GetLogicalDevice(), &commandPoolCI, VulkanContext::GetAllocator(), &VulkanContext::s_CommandPool));
			}

			// Create Command buffers
			{
				VkCommandBufferAllocateInfo cmdBufAllocInfo = {};
				cmdBufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				cmdBufAllocInfo.commandPool = VulkanContext::GetCommandPool();
				cmdBufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
				cmdBufAllocInfo.commandBufferCount = Renderer::GetFramesInFlight();

				m_VkCommandBuffers.resize(Renderer::GetFramesInFlight());
				VK_CHECK(vkAllocateCommandBuffers(VulkanContext::GetLogicalDevice(), &cmdBufAllocInfo, m_VkCommandBuffers.data()));
			}
		});

	}

	void VulkanRenderer::Shutdown()
	{
		Renderer::SubmitResourceFree([debugReport = m_DebugReport, vkCommandBuffers = m_VkCommandBuffers]()
		{
			VkDevice logicalDevice = VulkanContext::GetLogicalDevice();

			vkFreeCommandBuffers(logicalDevice, VulkanContext::GetCommandPool(), vkCommandBuffers.size(), vkCommandBuffers.data());
			vkDestroyCommandPool(logicalDevice, VulkanContext::GetCommandPool(), VulkanContext::GetAllocator());

			for (uint32_t i = 0; i < Renderer::GetFramesInFlight(); i++)
			{
				vkDestroySemaphore(logicalDevice, VulkanContext::GetFrameSyncData(i).ImageAcquiredSemaphore, VulkanContext::GetAllocator());
				vkDestroySemaphore(logicalDevice, VulkanContext::GetFrameSyncData(i).RenderCompleteSemaphore, VulkanContext::GetAllocator());

				vkDestroyFence(logicalDevice, VulkanContext::GetFrameSyncData(i).RenderCompleteFence, VulkanContext::GetAllocator());
			}

	#ifdef ATN_DEBUG
			auto vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(VulkanContext::GetInstance(), "vkDestroyDebugReportCallbackEXT");
			vkDestroyDebugReportCallbackEXT(VulkanContext::GetInstance(), debugReport, VulkanContext::GetAllocator());
	#endif

			// Destroy Device
			VulkanContext::s_CurrentDevice.Release();

			vkDestroyInstance(VulkanContext::GetInstance(), VulkanContext::GetAllocator());
		});
	}

	void VulkanRenderer::BeginFrame()
	{
		Renderer::Submit([this]()
		{
			ATN_PROFILE_SCOPE("VulkanRenderer::BeginFrame")
			VkCommandBuffer commandBuffer = m_VkCommandBuffers[Renderer::GetCurrentFrameIndex()];

			VK_CHECK(vkResetCommandBuffer(commandBuffer, 0));

			VkCommandBufferBeginInfo cmdBufBeginInfo = {};
			cmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			cmdBufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			VK_CHECK(vkBeginCommandBuffer(commandBuffer, &cmdBufBeginInfo));

			VulkanContext::SetActiveCommandBuffer(commandBuffer);
		});
	}

	void VulkanRenderer::EndFrame()
	{
		Renderer::Submit([this]()
		{
			ATN_PROFILE_SCOPE("VulkanRenderer::EndFrame")

			VulkanContext::SetActiveCommandBuffer(VK_NULL_HANDLE);

			VkCommandBuffer commandBuffer = m_VkCommandBuffers[Renderer::GetCurrentFrameIndex()];
			const FrameSyncData& frameData = VulkanContext::GetFrameSyncData(Renderer::GetCurrentFrameIndex());

			VK_CHECK(vkEndCommandBuffer(commandBuffer));

			bool enableUI = Application::Get().GetConfig().EnableImGui;
			VkPipelineStageFlags waitStage = enableUI ? VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT : VK_PIPELINE_STAGE_TRANSFER_BIT;

			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.waitSemaphoreCount = 1;
			submitInfo.pWaitSemaphores = &frameData.ImageAcquiredSemaphore;
			submitInfo.pWaitDstStageMask = &waitStage;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &commandBuffer;
			submitInfo.signalSemaphoreCount = 1;
			submitInfo.pSignalSemaphores = &frameData.RenderCompleteSemaphore;

			{
				ATN_PROFILE_SCOPE("vkQueueSubmit")
				auto& appstats = Application::Get().GetStats();
				appstats.Timer.Reset();

				VK_CHECK(vkQueueSubmit(VulkanContext::GetDevice()->GetQueue(), 1, &submitInfo, frameData.RenderCompleteFence));

				appstats.Renderer_QueueSubmit = appstats.Timer.ElapsedTime();
			}
		});
	}

	void VulkanRenderer::WaitDeviceIdle()
	{
		vkDeviceWaitIdle(VulkanContext::GetDevice()->GetLogicalDevice());
	}

	void VulkanRenderer::CopyTextureToSwapChain(const Ref<Texture2D>& texture)
	{
		ATN_PROFILE_FUNC()
		VkCommandBuffer commandBuffer = VulkanContext::GetActiveCommandBuffer();

		VkImage sourceImage = texture.As<VulkanTexture2D>()->GetVulkanImage();
		VkImage swapChainImage = Application::Get().GetWindow().GetSwapChain().As<VulkanSwapChain>()->GetCurrentVulkanImage();

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		{
			barrier.image = sourceImage;
			barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

			vkCmdPipelineBarrier(
				commandBuffer,
				sourceStage, destinationStage,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);

			barrier.image = swapChainImage;
			barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_NONE;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

			vkCmdPipelineBarrier(
				commandBuffer,
				sourceStage, destinationStage,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);
		}

		VkImageBlit imageBlitRegion = {};
		imageBlitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlitRegion.srcSubresource.layerCount = 1;
		imageBlitRegion.srcOffsets[1] = { (int)texture->GetInfo().Width, (int)texture->GetInfo().Height, 1 };
		imageBlitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlitRegion.dstSubresource.layerCount = 1;
		imageBlitRegion.dstOffsets[1] = { (int)texture->GetInfo().Width, (int)texture->GetInfo().Height, 1 };

		vkCmdBlitImage(
			commandBuffer,
			sourceImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			swapChainImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&imageBlitRegion,
			VK_FILTER_NEAREST);


		{
			barrier.image = sourceImage;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_NONE;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

			vkCmdPipelineBarrier(
				commandBuffer,
				sourceStage, destinationStage,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);


			barrier.image = swapChainImage;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_NONE;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

			vkCmdPipelineBarrier(
				commandBuffer,
				sourceStage, destinationStage,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);
		}
	}
}
