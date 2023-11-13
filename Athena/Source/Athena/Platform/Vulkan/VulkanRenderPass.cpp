#include "VulkanRenderPass.h"

#include "Athena/Platform/Vulkan/VulkanUtils.h"
#include "Athena/Platform/Vulkan/VulkanFramebuffer.h"


namespace Athena
{
	namespace VulkanUtils
	{
		static VkImageLayout GetAttachmentImageLayout(TextureFormat format)
		{
			if (Texture::IsColorFormat(format))
				return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			if (Texture::IsDepthFormat(format) || Texture::IsStencilFormat(format))
				return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			ATN_CORE_ASSERT(false);
			return (VkImageLayout)0;
		}

		static VkClearValue GetClearValue(const FramebufferAttachmentInfo& info)
		{
			VkClearValue result = {};

			if (Texture::IsColorFormat(info.Format))
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

		Renderer::Submit([this]()
		{
			std::vector<VkAttachmentDescription> attachments;
			std::vector<VkAttachmentReference> colorAttachmentRefs;

			bool hasDepthStencil = false;
			VkAttachmentReference depthStencilAttachmentRef = {};

			for (const auto& attachment : m_Info.Output->GetInfo().Attachments)
			{
				VkAttachmentDescription attachmentDesc = {};
				attachmentDesc.format = VulkanUtils::GetFormat(attachment.Format);
				attachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;

				VkAttachmentLoadOp loadOp = m_Info.LoadOpClear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
				VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE;

				attachmentDesc.loadOp = loadOp;
				attachmentDesc.storeOp = storeOp;
				attachmentDesc.stencilLoadOp = loadOp;
				attachmentDesc.stencilStoreOp = storeOp;

				attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				attachmentDesc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

				attachments.push_back(attachmentDesc);

				if (Texture::IsColorFormat(attachment.Format))
				{
					VkAttachmentReference colorAttachmentRef;
					colorAttachmentRef.attachment = attachments.size() - 1;
					colorAttachmentRef.layout = VulkanUtils::GetAttachmentImageLayout(attachment.Format);

					colorAttachmentRefs.push_back(colorAttachmentRef);
				}
				else
				{
					depthStencilAttachmentRef.attachment = attachments.size() - 1;
					depthStencilAttachmentRef.layout = VulkanUtils::GetAttachmentImageLayout(attachment.Format);
					hasDepthStencil = true;
				}
			}

			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;	// TODO: compute pipelines
			subpass.colorAttachmentCount = colorAttachmentRefs.size();
			subpass.pColorAttachments = colorAttachmentRefs.data();
			subpass.pDepthStencilAttachment = hasDepthStencil ? &depthStencilAttachmentRef : nullptr;

			VkRenderPassCreateInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassInfo.attachmentCount = attachments.size();
			renderPassInfo.pAttachments = attachments.data();
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;

			VK_CHECK(vkCreateRenderPass(VulkanContext::GetLogicalDevice(), &renderPassInfo, nullptr, &m_VulkanRenderPass));

			m_Info.Output.As<VulkanFramebuffer>()->RT_BakeFramebuffer(m_VulkanRenderPass);
		});
	}

	VulkanRenderPass::~VulkanRenderPass()
	{
		Renderer::SubmitResourceFree([renderPass = m_VulkanRenderPass]()
		{
			vkDestroyRenderPass(VulkanContext::GetLogicalDevice(), renderPass, nullptr);
		});
	}

	void VulkanRenderPass::Begin()
	{
		Renderer::Submit([this]()
		{
			uint32 width = m_Info.Output->GetInfo().Width;
			uint32 height = m_Info.Output->GetInfo().Height;

			VkRenderPassBeginInfo renderPassBeginInfo = {};
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.renderPass = m_VulkanRenderPass;
			renderPassBeginInfo.framebuffer = m_Info.Output.As<VulkanFramebuffer>()->GetVulkanFramebuffer();
			renderPassBeginInfo.renderArea.offset = { 0, 0 };
			renderPassBeginInfo.renderArea.extent = { width , height };

			std::vector<VkClearValue> clearValues;
			if (m_Info.LoadOpClear)
			{
				renderPassBeginInfo.clearValueCount = m_Info.Output->GetInfo().Attachments.size();
				clearValues.reserve(renderPassBeginInfo.clearValueCount);

				for (const auto& attachment : m_Info.Output->GetInfo().Attachments)
				{
					VkClearValue clearValue = VulkanUtils::GetClearValue(attachment);
					clearValues.push_back(clearValue);
				}

				renderPassBeginInfo.pClearValues = clearValues.data();
			}

			vkCmdBeginRenderPass(VulkanContext::GetActiveCommandBuffer(), &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		});
	}

	void VulkanRenderPass::End()
	{
		Renderer::Submit([this]()
		{
			vkCmdEndRenderPass(VulkanContext::GetActiveCommandBuffer());
		});
	}
}
