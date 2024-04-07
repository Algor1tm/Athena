#include "VulkanRenderPass.h"

#include "Athena/Platform/Vulkan/VulkanUtils.h"
#include "Athena/Platform/Vulkan/VulkanTexture2D.h"
#include "Athena/Platform/Vulkan/VulkanImage.h"
#include "Athena/Platform/Vulkan/VulkanRenderCommandBuffer.h"


namespace Athena
{
	namespace Vulkan
	{
		static VkAttachmentLoadOp GetLoadOp(RenderTargetLoadOp loadOp)
		{
			switch (loadOp)
			{
			case RenderTargetLoadOp::DONT_CARE: return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			case RenderTargetLoadOp::CLEAR: return VK_ATTACHMENT_LOAD_OP_CLEAR;
			case RenderTargetLoadOp::LOAD: return VK_ATTACHMENT_LOAD_OP_LOAD;
			}

			ATN_CORE_ASSERT(false);
			return (VkAttachmentLoadOp)0;
		}

		static VkImageLayout GetAttachmentOptimalLayout(TextureFormat format)
		{
			if (Texture::IsColorFormat(format))
				return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			if (Texture::IsDepthFormat(format) || Texture::IsStencilFormat(format))
				return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			ATN_CORE_ASSERT(false);
			return (VkImageLayout)0;
		}

		static VkClearValue GetClearValue(const RenderTarget& info)
		{
			VkClearValue result = {};

			if (Texture::IsColorFormat(info.Texture->GetFormat()))
			{
				result.color = { info.ClearColor[0], info.ClearColor[1], info.ClearColor[2], info.ClearColor[3] };
			}
			else
			{
				result.depthStencil = { info.DepthClearColor, info.StencilClearColor };
			}
			
			return result;
		}
	}

	VulkanRenderPass::VulkanRenderPass(const RenderPassCreateInfo& info)
	{
		m_Info = info;
	}

	VulkanRenderPass::~VulkanRenderPass()
	{
		Renderer::SubmitResourceFree([renderPass = m_VulkanRenderPass, framebuffer = m_VulkanFramebuffer]()
		{
			vkDestroyFramebuffer(VulkanContext::GetLogicalDevice(), framebuffer, nullptr);
			vkDestroyRenderPass(VulkanContext::GetLogicalDevice(), renderPass, nullptr);
		});
	}

	void VulkanRenderPass::Begin(const Ref<RenderCommandBuffer>& commandBuffer)
	{
		if(m_Info.DebugColor != LinearColor(0.f))
			Renderer::BeginDebugRegion(commandBuffer, m_Info.Name, m_Info.DebugColor);

		for (const auto& output : m_Outputs)
			output.Texture.As<VulkanTexture2D>()->GetImage().As<VulkanImage>()->RenderPassUpdateLayout(Vulkan::GetAttachmentOptimalLayout(output.Texture->GetFormat()));

		VkCommandBuffer vkcmdBuf = commandBuffer.As<VulkanRenderCommandBuffer>()->GetActiveCommandBuffer();
		uint32 width = m_Info.Width;
		uint32 height = m_Info.Height;

		VkRenderPassBeginInfo renderPassBeginInfo = {};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.renderPass = m_VulkanRenderPass;
		renderPassBeginInfo.framebuffer = m_VulkanFramebuffer;
		renderPassBeginInfo.renderArea.offset = { 0, 0 };
		renderPassBeginInfo.renderArea.extent = { width , height };
		renderPassBeginInfo.clearValueCount = m_ClearColors.size();
		renderPassBeginInfo.pClearValues = m_ClearColors.data();

		vkCmdBeginRenderPass(vkcmdBuf, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	}

	void VulkanRenderPass::End(const Ref<RenderCommandBuffer>& commandBuffer)
	{
		VkCommandBuffer vkcmdBuf = commandBuffer.As<VulkanRenderCommandBuffer>()->GetActiveCommandBuffer();
		vkCmdEndRenderPass(vkcmdBuf);
		
		for (const auto& output : m_Outputs)
			output.Texture.As<VulkanTexture2D>()->GetImage().As<VulkanImage>()->RenderPassUpdateLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		if (m_Info.DebugColor != LinearColor(0.f))
			Renderer::EndDebugRegion(commandBuffer);
	}

	void VulkanRenderPass::Bake()
	{
		std::vector<VkAttachmentDescription> attachments;
		std::vector<VkAttachmentReference> colorAttachmentRefs;

		bool hasDepthStencil = false;
		VkAttachmentReference depthStencilAttachmentRef = {};

		for (const auto& attachment : m_Outputs)
		{
			TextureFormat format = attachment.Texture->GetFormat();

			VkAttachmentDescription attachmentDesc = {};
			attachmentDesc.format = Vulkan::GetFormat(format);
			attachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;

			VkAttachmentLoadOp loadOp = Vulkan::GetLoadOp(attachment.LoadOp);
			VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE;

			attachmentDesc.loadOp = loadOp;
			attachmentDesc.storeOp = storeOp;
			attachmentDesc.stencilLoadOp = loadOp;
			attachmentDesc.stencilStoreOp = storeOp;

			attachmentDesc.initialLayout = attachment.LoadOp == RenderTargetLoadOp::LOAD ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
			attachmentDesc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			attachments.push_back(attachmentDesc);

			if (Texture::IsColorFormat(format))
			{
				VkAttachmentReference colorAttachmentRef;
				colorAttachmentRef.attachment = attachments.size() - 1;
				colorAttachmentRef.layout = Vulkan::GetAttachmentOptimalLayout(format);

				colorAttachmentRefs.push_back(colorAttachmentRef);
			}
			else
			{
				ATN_CORE_ASSERT(!hasDepthStencil, "Max 1 depth attachment in framebuffer!");

				depthStencilAttachmentRef.attachment = attachments.size() - 1;
				depthStencilAttachmentRef.layout = Vulkan::GetAttachmentOptimalLayout(format);
				hasDepthStencil = true;
			}
		}

		std::vector<VkSubpassDependency> dependencies;
		BuildDependencies(dependencies);

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = colorAttachmentRefs.size();
		subpass.pColorAttachments = colorAttachmentRefs.data();
		subpass.pDepthStencilAttachment = hasDepthStencil ? &depthStencilAttachmentRef : nullptr;

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = attachments.size();
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = dependencies.size();
		renderPassInfo.pDependencies = dependencies.data();

		VK_CHECK(vkCreateRenderPass(VulkanContext::GetLogicalDevice(), &renderPassInfo, nullptr, &m_VulkanRenderPass));
		Vulkan::SetObjectDebugName(m_VulkanRenderPass, VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT, m_Info.Name);

		for (const auto& attachment : m_Outputs)
		{
			if (attachment.LoadOp == RenderTargetLoadOp::CLEAR)
			{
				VkClearValue clearValue = Vulkan::GetClearValue(attachment);
				m_ClearColors.push_back(clearValue);
			}
		}

		uint32 width = m_Info.Width;
		uint32 height = m_Info.Height;

		m_Info.Width = 0;
		m_Info.Height = 0;
		Resize(width, height);
	}

	void VulkanRenderPass::Resize(uint32 width, uint32 height)
	{
		if (m_Info.Width == width && m_Info.Height == height)
			return;

		m_Info.Width = width;
		m_Info.Height = height;

		if (m_VulkanFramebuffer)
		{
			Renderer::SubmitResourceFree([framebuffer = m_VulkanFramebuffer]()
			{
				vkDestroyFramebuffer(VulkanContext::GetLogicalDevice(), framebuffer, nullptr);
			});
		}

		for (auto& attachment : m_Outputs)
			attachment.Texture->Resize(width, height);

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_VulkanRenderPass;
		framebufferInfo.attachmentCount = m_Outputs.size();
		framebufferInfo.width = m_Info.Width;
		framebufferInfo.height = m_Info.Height;
		framebufferInfo.layers = m_Info.Layers;

		std::vector<VkImageView> attachmentViews;

		for (auto& attachment : m_Outputs)
		{
			VkImageView view = attachment.Texture.As<VulkanTexture2D>()->GetVulkanImageView();
			attachmentViews.push_back(view);
		}

		framebufferInfo.attachmentCount = attachmentViews.size();
		framebufferInfo.pAttachments = attachmentViews.data();

		VK_CHECK(vkCreateFramebuffer(VulkanContext::GetLogicalDevice(), &framebufferInfo, nullptr, &m_VulkanFramebuffer));
		Vulkan::SetObjectDebugName(m_VulkanFramebuffer, VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT, std::format("{}_FB", m_Info.Name));
	}

	void VulkanRenderPass::BuildDependencies(std::vector<VkSubpassDependency>& dependencies)
	{
		if (!m_Info.InputPass)
			return;

		bool hasSharedColorTarget = false;
		bool hasSharedDepthTarget = false;

		// Check if we have output target from previous render pass
		for (const auto& inputTarget : m_Info.InputPass->GetAllOutputs())
		{
			for (const auto& outputTarget : m_Outputs)
			{
				if (outputTarget.Texture == inputTarget.Texture)
				{
					if (Texture::IsColorFormat(outputTarget.Texture->GetFormat()))
						hasSharedColorTarget = true;
					else
						hasSharedDepthTarget = true;
				}
			}
		}

		// Color Attachment
		if (hasSharedColorTarget)
		{
			VkSubpassDependency dependency = {};
			dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
			dependency.dstSubpass = 0;
			dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
			dependencies.push_back(dependency);
		}
		// Shader access
		else if(m_Info.InputPass->GetColorTargetsCount() > 0)
		{
			VkSubpassDependency dependency = {};
			dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
			dependency.dstSubpass = 0;
			dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			dependency.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
			dependencies.push_back(dependency);
		}

		// Depth Attachment
		if (hasSharedDepthTarget)
		{
			VkSubpassDependency dependency = {};
			dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
			dependency.dstSubpass = 0;
			dependency.srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
			dependencies.push_back(dependency);
		}
		// Shader access
		else if(m_Info.InputPass->HasDepthRenderTarget())
		{
			VkSubpassDependency dependency = {};
			dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
			dependency.dstSubpass = 0;
			dependency.srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			dependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			dependency.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
			dependencies.push_back(dependency);
		}
	}
}
