#include "VulkanImGuiBackend.h"

#include "Athena/Core/Application.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanRHI.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanUtils.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanSwapChain.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanTexture.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanTracyZone.h"
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
			framebuffers = m_SwapChainFramebuffers, textureQueue = m_ImGuiTextureQueue]() mutable
			{
				textureQueue.Shutdown();

				ImGui_ImplVulkan_Shutdown();
				ImGui_ImplGlfw_Shutdown();

				vkDestroyDescriptorPool(Vulkan::GetLogicalDevice(), descPool, nullptr);
				vkDestroyRenderPass(Vulkan::GetLogicalDevice(), renderPass, nullptr);

				for (auto framebuffer : framebuffers)
					vkDestroyFramebuffer(Vulkan::GetLogicalDevice(), framebuffer, nullptr);

				ImGui::DestroyContext();
			});
	}

	void VulkanImGuiBackend::Init_GT(void* windowHandle)
	{
		m_ImGuiTextureQueue.InFlightFrames = Vulkan::GetMaxFramesInFlight();
		m_ImGuiTextureQueue.UpdateTexFunc = ImGui_ImplVulkan_UpdateTexture;

		ImGui_ImplGlfw_InitForVulkan(reinterpret_cast<GLFWwindow*>(windowHandle), true);
		ImGui_ImplVulkan_Init_GT();
	}

	void VulkanImGuiBackend::Init_RT()
	{
		VkDescriptorPoolSize pool_sizes[] = {
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

		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.ApiVersion = VulkanRHI::Get()->GetVulkanVersion();
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

	void VulkanImGuiBackend::NewFrame_GT()
	{
		std::unique_lock lock(m_TextureQueueMutex);
		m_ImGuiTextureQueue.PreNewFrame();
	}

	void VulkanImGuiBackend::NewFrame_RT()
	{
		TRACY_PROFILE_FUNC();

		InvalidateDescriptorSets();
		ImGui_ImplVulkan_NewFrame();

		ImGuiContext& g = *GImGui;
		for (ImGuiViewportP* viewport : g.Viewports)
		{
			viewport->SnapshotIndex_RT = (viewport->SnapshotIndex_RT + 1) % 2;
		}
	}

	void VulkanImGuiBackend::SwapDrawData()
	{
		TRACY_PROFILE_FUNC();

		ImGuiContext& g = *GImGui;

		{
			// Get draw data from main viewport (index 0), typically texture lists are shared between viewports draw data
			ImGuiViewportP* viewport = g.Viewports[0];

			std::unique_lock lock(m_TextureQueueMutex);
			m_ImGuiTextureQueue.QueueRequests(&viewport->DrawDataP);
		}

		// Snapshot draw data for render thread
		for (ImGuiViewportP* viewport : g.Viewports)
		{
			viewport->SnapshotIndex_GT = (viewport->SnapshotIndex_GT + 1) % 2;
			viewport->Snapshots[viewport->SnapshotIndex_GT].DrawDataSnapshot.SnapUsingSwap(&viewport->DrawDataP, ImGui::GetTime());
			viewport->Snapshots[viewport->SnapshotIndex_GT].Size = viewport->Size;
			viewport->Snapshots[viewport->SnapshotIndex_GT].Flags = viewport->Flags;
		}
	}

	void VulkanImGuiBackend::RenderDrawData()
	{
		TRACY_PROFILE_FUNC();

		if (!Application::Get().GetWindow().GetSwapChain())
			return;

		Ref<VulkanSwapChain> swapChain = Application::Get().GetWindow().GetSwapChain().As<VulkanSwapChain>();
		VulkanCommandBuffer* commandBuffer = VulkanRHI::Get()->GetCmdBufferManager()->GetPresentCommandBuffer();

		uint32 width = swapChain->GetWidth();
		uint32 height = swapChain->GetHeight();

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
			TRACY_VULKAN_ZONE(commandBuffer->CommandBuffer, "ImGui Pass");
			VK_BEGIN_DEBUG_LABEL(commandBuffer->CommandBuffer, "ImGui Pass", IntegerColor::Magenta);

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

			ImDrawData* drawData = ImGui::GetDrawData();
			{
				std::unique_lock lock(m_TextureQueueMutex);
				m_ImGuiTextureQueue.ProcessRequests(drawData);
			}

			ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffer->CommandBuffer);

			vkCmdEndRenderPass(commandBuffer->CommandBuffer);
			VK_END_DEBUG_LABEL(commandBuffer->CommandBuffer);
		}

		VulkanRHI::Get()->GetCmdBufferManager()->SubmitForPresent(swapChain->GetCurrentImageIndex(), swapChain->GetImageAcquiredSemaphore(), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
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

		Ref<VulkanSwapChain> vkSwapChain = Application::Get().GetWindow().GetSwapChain().As<VulkanSwapChain>();

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
			return GetTextureID(EngineRenderAssets::Get().WhiteTexture->GetView());

		{
			std::unique_lock lock(m_TexturesMapMutex);

			if (m_TextureViewsMap.contains(view))
				return m_TextureViewsMap.at(view).Set;
		}

		ENQUEUE_RENDER_COMMAND([this, viewRT = view]() 
			{
				UpdateTextureID_RT(viewRT);
			});

		return VK_NULL_HANDLE;
	}

	void VulkanImGuiBackend::UpdateTextureID_RT(const Ref<TextureView>& view)
	{
		TRACY_PROFILE_FUNC();

		{
			std::unique_lock lock(m_TexturesMapMutex);
			if (m_TextureViewsMap.contains(view))
				return;
		}

		TextureInfo info;
		info.VulkanImageView = view.As<VulkanTextureView>()->GetImageView();
		info.VulkanSampler = view.As<VulkanTextureView>()->GetSampler();

		if (info.VulkanImageView == VK_NULL_HANDLE)
		{
			info.VulkanImageView = EngineRenderAssets::Get().WhiteTexture->GetView().As<VulkanTextureView>()->GetImageView();
		}

		info.Set = ImGui_ImplVulkan_AddTexture(info.VulkanSampler, info.VulkanImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		std::unique_lock lock(m_TexturesMapMutex);
		m_TextureViewsMap[view] = info;
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

				VkDescriptorSet set = ImGui_ImplVulkan_AddTexture(sampler, imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

				std::unique_lock lock(m_TexturesMapMutex);
				info.VulkanImageView = imageView;
				info.VulkanSampler = sampler;
				info.Set = set;
			}
		}

		std::unique_lock lock(m_TexturesMapMutex);
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
