#include "VulkanImGuiLayerImpl.h"

#include "Athena/Core/Application.h"

#include "Athena/Platform/Vulkan/VulkanUtils.h"
#include "Athena/Platform/Vulkan/VulkanSwapChain.h"
#include "Athena/Platform/Vulkan/VulkanTexture2D.h"
#include "Athena/Platform/Vulkan/VulkanImage.h"
#include "Athena/Platform/Vulkan/VulkanRenderCommandBuffer.h"

#include <ImGui/backends/imgui_impl_glfw.h>
#include <ImGui/backends/imgui_impl_vulkan.h>


namespace Athena
{
	void VulkanImGuiLayerImpl::Init(void* windowHandle)
	{
		VkDescriptorPoolSize pool_sizes[] =
		{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
		};

		VkDescriptorPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		pool_info.maxSets = 1000;
		pool_info.poolSizeCount = std::size(pool_sizes);
		pool_info.pPoolSizes = pool_sizes;

		VK_CHECK(vkCreateDescriptorPool(VulkanContext::GetDevice()->GetLogicalDevice(), &pool_info, nullptr, &m_ImGuiDescriptorPool));

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
			dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency.srcAccessMask = 0;
			dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			VkRenderPassCreateInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassInfo.attachmentCount = 1;
			renderPassInfo.pAttachments = &colorAttachment;
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;
			renderPassInfo.dependencyCount = 1;
			renderPassInfo.pDependencies = &dependency;

			VK_CHECK(vkCreateRenderPass(VulkanContext::GetDevice()->GetLogicalDevice(), &renderPassInfo, nullptr, &m_ImGuiRenderPass));
			Vulkan::SetObjectDebugName(m_ImGuiRenderPass, VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT, "ImGuiRenderPass");
		}

		RecreateFramebuffers();

		ImGui_ImplGlfw_InitForVulkan(reinterpret_cast<GLFWwindow*>(windowHandle), true);

		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = VulkanContext::GetInstance();
		init_info.PhysicalDevice = VulkanContext::GetDevice()->GetPhysicalDevice();
		init_info.Device = VulkanContext::GetDevice()->GetLogicalDevice();
		init_info.QueueFamily = VulkanContext::GetDevice()->GetQueueFamily();
		init_info.Queue = VulkanContext::GetDevice()->GetQueue();
		init_info.PipelineCache = VK_NULL_HANDLE;
		init_info.DescriptorPool = m_ImGuiDescriptorPool;
		init_info.Subpass = 0;
		init_info.MinImageCount = Renderer::GetFramesInFlight();
		init_info.ImageCount = Renderer::GetFramesInFlight();
		init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		init_info.Allocator = nullptr;
		init_info.CheckVkResultFn = [](VkResult result) { Vulkan::CheckResult(result); ATN_CORE_ASSERT(result == VK_SUCCESS) };

		ImGui_ImplVulkan_Init(&init_info, m_ImGuiRenderPass);

		VkCommandBuffer vkCommandBuffer = Vulkan::BeginSingleTimeCommands();
		{
			ImGui_ImplVulkan_CreateFontsTexture(vkCommandBuffer);
		}
		Vulkan::EndSingleTimeCommands(vkCommandBuffer);

		ImGui_ImplVulkan_DestroyFontUploadObjects();

		VkSamplerCreateInfo vksamplerInfo = {};
		vksamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		vksamplerInfo.magFilter = VK_FILTER_LINEAR;
		vksamplerInfo.minFilter = VK_FILTER_LINEAR;
		vksamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		vksamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		vksamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		vksamplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		vksamplerInfo.anisotropyEnable = false;
		vksamplerInfo.maxAnisotropy = 1.f;
		vksamplerInfo.compareEnable = false;
		vksamplerInfo.compareOp = VK_COMPARE_OP_NEVER;
		vksamplerInfo.minLod = 0.f;
		vksamplerInfo.maxLod = 1;
		vksamplerInfo.mipLodBias = 0.f;
		vksamplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
		vksamplerInfo.unnormalizedCoordinates = false;

		VK_CHECK(vkCreateSampler(VulkanContext::GetLogicalDevice(), &vksamplerInfo, nullptr, &m_DefaultUISampler));
		Vulkan::SetObjectDebugName(m_DefaultUISampler, VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT, "Sampler_DefaultUI");
	}

	void VulkanImGuiLayerImpl::Shutdown()
	{
		m_ImageMap.clear();

		Renderer::SubmitResourceFree([descPool = m_ImGuiDescriptorPool, renderPass = m_ImGuiRenderPass, 
			framebuffers = m_SwapChainFramebuffers, uiSampler = m_DefaultUISampler]()
		{
			vkDestroySampler(VulkanContext::GetLogicalDevice(), uiSampler, nullptr);
			vkDestroyDescriptorPool(VulkanContext::GetLogicalDevice(), descPool, nullptr);
			vkDestroyRenderPass(VulkanContext::GetLogicalDevice(), renderPass, nullptr);

			for (auto framebuffer : framebuffers)
				vkDestroyFramebuffer(VulkanContext::GetLogicalDevice(), framebuffer, nullptr);

			ImGui_ImplVulkan_Shutdown();
			ImGui_ImplGlfw_Shutdown();
		});
	}

	void VulkanImGuiLayerImpl::NewFrame()
	{
		ATN_PROFILE_FUNC()

		InvalidateDescriptorSets();

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
	}

	void VulkanImGuiLayerImpl::RenderDrawData(uint32 width, uint32 height)
	{
		ATN_PROFILE_FUNC()

		Ref<SwapChain> swapChain = Application::Get().GetWindow().GetSwapChain();
		VkCommandBuffer commandBuffer = Renderer::GetRenderCommandBuffer().As<VulkanRenderCommandBuffer>()->GetVulkanCommandBuffer();

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_ImGuiRenderPass;
		renderPassInfo.framebuffer = m_SwapChainFramebuffers[swapChain->GetCurrentImageIndex()];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = { width, height };
		VkClearValue clearColor = { { 0.f, 0.f, 0.f, 1.0f } };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

		vkCmdEndRenderPass(commandBuffer);
	}

	void VulkanImGuiLayerImpl::OnSwapChainRecreate()
	{
		ATN_PROFILE_FUNC()
		RecreateFramebuffers();
	}

	void VulkanImGuiLayerImpl::RecreateFramebuffers()
	{
		Renderer::SubmitResourceFree([framebuffers = m_SwapChainFramebuffers]()
		{
			for (auto framebuffer : framebuffers)
				vkDestroyFramebuffer(VulkanContext::GetDevice()->GetLogicalDevice(), framebuffer, nullptr);
		});

		m_SwapChainFramebuffers.resize(Renderer::GetFramesInFlight());

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

			VK_CHECK(vkCreateFramebuffer(VulkanContext::GetDevice()->GetLogicalDevice(), &framebufferInfo, nullptr, &m_SwapChainFramebuffers[i]));
		}
	}

	void* VulkanImGuiLayerImpl::GetTextureID(const Ref<Texture2D>& texture)
	{
		if (texture == nullptr)
			return GetTextureID(Renderer::GetWhiteTexture()->GetImage());

		// Check sampler specifically for textures
		if (m_ImageMap.contains(texture->GetImage()))
		{
			VkSampler sampler = texture.As<VulkanTexture2D>()->GetVulkanSampler();
			auto& imageInfo = m_ImageMap.at(texture->GetImage());

			if (sampler != imageInfo.Sampler)
			{
				RemoveDescriptorSet(imageInfo.Set);
				imageInfo.Sampler = sampler;
				imageInfo.Set = ImGui_ImplVulkan_AddTexture(imageInfo.Sampler, imageInfo.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			}

			return imageInfo.Set;
		}

		return GetTextureID(texture->GetImage());
	}

	void* VulkanImGuiLayerImpl::GetTextureID(const Ref<Image>& image)
	{
		if (image == nullptr)
			return GetTextureID(Renderer::GetWhiteTexture());

		if (m_ImageMap.contains(image))
			return m_ImageMap.at(image).Set;

		ImageInfo info;
		info.ImageView = image.As<VulkanImage>()->GetVulkanImageView();
		info.Sampler = m_DefaultUISampler;

		if (info.ImageView == nullptr || info.Sampler == nullptr)
			return GetTextureID(Renderer::GetWhiteTexture());

		info.Set = ImGui_ImplVulkan_AddTexture(info.Sampler, info.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		m_ImageMap[image] = info;

		return info.Set;
	}

	void VulkanImGuiLayerImpl::InvalidateDescriptorSets()
	{
		std::vector<Ref<Image>> imagesToRemove;
		for (auto& [image, imageInfo] : m_ImageMap)
		{
			// All instances of texture has been deleted except one in texture map
			if (image->GetCount() == 1)
			{
				RemoveDescriptorSet(imageInfo.Set);
				imagesToRemove.push_back(image);
			}

			VkImageView imageView = image.As<VulkanImage>()->GetVulkanImageView();

			// Check if texture has been modified and update descriptor set
			if (imageInfo.ImageView != imageView)
			{
				RemoveDescriptorSet(imageInfo.Set);
				imageInfo.ImageView = imageView;
				imageInfo.Set = ImGui_ImplVulkan_AddTexture(imageInfo.Sampler, imageInfo.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			}
		}

		for (const auto& texture : imagesToRemove)
			m_ImageMap.erase(texture);
	}

	void VulkanImGuiLayerImpl::RemoveDescriptorSet(VkDescriptorSet set)
	{
		Renderer::SubmitResourceFree([set = set]()
		{
			ImGui_ImplVulkan_RemoveTexture(set);
		});
	}
}
