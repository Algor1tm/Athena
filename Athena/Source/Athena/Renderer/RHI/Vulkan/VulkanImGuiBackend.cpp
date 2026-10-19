#include "VulkanImGuiBackend.h"

#include "Athena/Core/Application.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanRHI.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanUtils.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanSwapChain.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanTextureView.h"
#include "Athena/Renderer/EngineRenderAssets.h"

#include <ImGui/backends/imgui_impl_glfw.h>
#include <ImGui/backends/imgui_impl_vulkan.h>


namespace Athena
{
	VulkanImGuiBackend::VulkanImGuiBackend()
	{
		m_ImGuiDescriptorPool = VK_NULL_HANDLE;
		m_ImGuiRenderPass = VK_NULL_HANDLE;
	}

	VulkanImGuiBackend::~VulkanImGuiBackend()
	{
		m_TextureViewsMap.clear();

		VulkanRHI::Get()->DeleteFrameResourcesDeferred([descPool = m_ImGuiDescriptorPool, renderPass = m_ImGuiRenderPass,
			framebuffers = m_SwapChainFramebuffers]()
			{
				ImGui_ImplVulkan_Shutdown();
				ImGui_ImplGlfw_Shutdown();

				vkDestroyDescriptorPool(Vulkan::GetLogicalDevice(), descPool, nullptr);
				vkDestroyRenderPass(Vulkan::GetLogicalDevice(), renderPass, nullptr);

				for (auto framebuffer : framebuffers)
					vkDestroyFramebuffer(Vulkan::GetLogicalDevice(), framebuffer, nullptr);

				ImGui::DestroyContext();
			});
	}

	void VulkanImGuiBackend::Init(void* windowHandle)
	{
		VkDescriptorPoolSize pool_sizes[] = {
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 512 },
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 128 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 512 },
		};

		VkDescriptorPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		pool_info.maxSets = 1000;
		pool_info.poolSizeCount = std::size(pool_sizes);
		pool_info.pPoolSizes = pool_sizes;

		VK_CHECK(vkCreateDescriptorPool(Vulkan::GetLogicalDevice(), &pool_info, nullptr, &m_ImGuiDescriptorPool));
		VK_SET_OBJECT_NAME(m_ImGuiDescriptorPool, VK_OBJECT_TYPE_DESCRIPTOR_POOL, "ImGuiDescriptorPool");

		// Create Render Pass
		{
			VkAttachmentDescription colorAttachment = {};
			colorAttachment.format = Application::Get().GetWindow().GetSwapChain().As<VulkanSwapChain>()->GetFormat();
			colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			VkAttachmentReference colorAttachmentRef = {};
			colorAttachmentRef.attachment = 0;
			colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &colorAttachmentRef;

			VkSubpassDependency dependency = {};
			dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
			dependency.dstSubpass = 0;
			dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			dependency.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			VkRenderPassCreateInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassInfo.attachmentCount = 1;
			renderPassInfo.pAttachments = &colorAttachment;
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;
			renderPassInfo.dependencyCount = 1;
			renderPassInfo.pDependencies = &dependency;

			VK_CHECK(vkCreateRenderPass(Vulkan::GetLogicalDevice(), &renderPassInfo, nullptr, &m_ImGuiRenderPass));
			VK_SET_OBJECT_NAME(m_ImGuiRenderPass, VK_OBJECT_TYPE_RENDER_PASS, "ImGuiRenderPass");
		}

		RecreateFramebuffers();

		ImGui_ImplGlfw_InitForVulkan(reinterpret_cast<GLFWwindow*>(windowHandle), true);

		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = VulkanRHI::Get()->GetVulkanInstance();
		init_info.PhysicalDevice = VulkanRHI::Get()->GetDevice()->GetPhysicalDevice();
		init_info.Device = VulkanRHI::Get()->GetDevice()->GetLogicalDevice();
		init_info.QueueFamily = VulkanRHI::Get()->GetDevice()->GetQueueFamily();
		init_info.Queue = VulkanRHI::Get()->GetDevice()->GetQueue();
		init_info.PipelineCache = VK_NULL_HANDLE;
		init_info.DescriptorPool = m_ImGuiDescriptorPool;
		init_info.MinImageCount = Vulkan::GetMaxFramesInFlight();
		init_info.ImageCount = Vulkan::GetMaxFramesInFlight();
		init_info.PipelineInfoMain.Subpass = 0;
		init_info.PipelineInfoMain.RenderPass = m_ImGuiRenderPass;
		init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		init_info.Allocator = nullptr;
		init_info.CheckVkResultFn = [](VkResult result) { Vulkan::CheckResult(result); ensuref(result == VK_SUCCESS) };

		ImGui_ImplVulkan_Init(&init_info);
	}

	void VulkanImGuiBackend::NewFrame()
	{
		TRACY_PROFILE_FUNC();

		InvalidateDescriptorSets();

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
	}

	void VulkanImGuiBackend::RenderDrawData(uint32 width, uint32 height)
	{
		TRACY_PROFILE_FUNC();

		Ref<VulkanSwapChain> swapChain = Application::Get().GetWindow().GetSwapChain().As<VulkanSwapChain>();
		VulkanCommandBuffer* commandBuffer = VulkanRHI::Get()->GetCmdBufferManager()->GetPresentCommandBuffer();

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_ImGuiRenderPass;
		renderPassInfo.framebuffer = m_SwapChainFramebuffers[swapChain->GetCurrentImageIndex()];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = { width, height };
		VkClearValue clearColor = { { 0.f, 0.f, 0.f, 1.0f } };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		{
			// Insert memory barrier rendering commands and UI
			VkMemoryBarrier memoryBarrier = {};
			memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
			memoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_SHADER_WRITE_BIT;
			memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(
				commandBuffer->CommandBuffer,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				0,
				1, &memoryBarrier,
				0, nullptr,
				0, nullptr
			);

			vkCmdBeginRenderPass(commandBuffer->CommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer->CommandBuffer);

			vkCmdEndRenderPass(commandBuffer->CommandBuffer);
		}

		VulkanRHI::Get()->GetCmdBufferManager()->SubmitForPresent(swapChain->GetImageAcquiredSemaphore(), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
	}

	void VulkanImGuiBackend::RenderViewports()
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}

	void VulkanImGuiBackend::OnSwapChainRecreate()
	{
		TRACY_PROFILE_FUNC();
		RecreateFramebuffers();
	}

	void VulkanImGuiBackend::RecreateFramebuffers()
	{
		VulkanRHI::Get()->DeleteFrameResourcesDeferred([framebuffers = m_SwapChainFramebuffers]()
			{
				for (auto framebuffer : framebuffers)
					vkDestroyFramebuffer(Vulkan::GetLogicalDevice(), framebuffer, nullptr);
			});

		m_SwapChainFramebuffers.resize(Vulkan::GetMaxFramesInFlight());

		if (!m_ImGuiRenderPass)
			return;

		Ref<VulkanSwapChain> vkSwapChain = Application::Get().GetWindow().GetSwapChain().As<VulkanSwapChain>();;

		for (size_t i = 0; i < vkSwapChain->GetVulkanImageViews().size(); i++)
		{
			VkImageView attachment = vkSwapChain->GetVulkanImageViews()[i];

			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = m_ImGuiRenderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = &attachment;
			framebufferInfo.width = Application::Get().GetWindow().GetWidth();
			framebufferInfo.height = Application::Get().GetWindow().GetHeight();
			framebufferInfo.layers = 1;

			VK_CHECK(vkCreateFramebuffer(Vulkan::GetLogicalDevice(), &framebufferInfo, nullptr, &m_SwapChainFramebuffers[i]));
			VK_SET_OBJECT_NAME(m_SwapChainFramebuffers[i], VK_OBJECT_TYPE_FRAMEBUFFER, std::format("ImGuiFrameBuffer_{}", i));
		}
	}

	void* VulkanImGuiBackend::GetTextureID(const Ref<TextureView>& view)
	{
		if (view == nullptr)
			return GetTextureID(EngineRenderAssets::Get().WhiteTexture->GetMainView());

		if (m_TextureViewsMap.contains(view))
			return m_TextureViewsMap.at(view).Set;

		TextureInfo info;
		info.VulkanImageView = view.As<VulkanTextureView>()->GetImageView();
		info.VulkanSampler = view.As<VulkanTextureView>()->GetSampler();

		if (info.VulkanImageView == VK_NULL_HANDLE)
			return GetTextureID(EngineRenderAssets::Get().WhiteTexture->GetMainView());

		info.Set = ImGui_ImplVulkan_AddTexture(info.VulkanSampler, info.VulkanImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		m_TextureViewsMap[view] = info;
		return info.Set;
	}

	void VulkanImGuiBackend::InvalidateDescriptorSets()
	{
		std::vector<Ref<TextureView>> viewsToRemove;
		for (auto& [view, info] : m_TextureViewsMap)
		{
			VkImageView imageView = view.As<VulkanTextureView>()->GetImageView();

			// All instances of texture has been deleted except one in texture map
			if (view->GetCount() == 1 || imageView == VK_NULL_HANDLE)
			{
				RemoveDescriptorSet(info.Set);
				viewsToRemove.push_back(view);
				continue;
			}

			VkSampler sampler = view.As<VulkanTextureView>()->GetSampler();

			// Check if texture has been modified and update descriptor set
			if (info.VulkanImageView != imageView || info.VulkanSampler != sampler)
			{
				RemoveDescriptorSet(info.Set);

				info.VulkanImageView = imageView;
				info.VulkanSampler = sampler;
				info.Set = ImGui_ImplVulkan_AddTexture(info.VulkanSampler, info.VulkanImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			}
		}

		for (const auto& view : viewsToRemove)
			m_TextureViewsMap.erase(view);
	}

	void VulkanImGuiBackend::RemoveDescriptorSet(VkDescriptorSet set)
	{
		VulkanRHI::Get()->DeleteFrameResourcesDeferred([set = set]()
			{
				ImGui_ImplVulkan_RemoveTexture(set);
			});
	}
}
