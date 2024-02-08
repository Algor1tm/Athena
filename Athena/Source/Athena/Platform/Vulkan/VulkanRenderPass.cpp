#include "VulkanRenderPass.h"

#include "Athena/Platform/Vulkan/VulkanUtils.h"
#include "Athena/Platform/Vulkan/VulkanFramebuffer.h"
#include "Athena/Platform/Vulkan/VulkanRenderCommandBuffer.h"


namespace Athena
{
	namespace Vulkan
	{
		static VkAttachmentLoadOp GetLoadOp(RenderPassLoadOp loadOp)
		{
			switch (loadOp)
			{
			case RenderPassLoadOp::DONT_CARE: return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			case RenderPassLoadOp::CLEAR: return VK_ATTACHMENT_LOAD_OP_CLEAR;
			case RenderPassLoadOp::LOAD: return VK_ATTACHMENT_LOAD_OP_LOAD;
			}

			ATN_CORE_ASSERT(false);
			return (VkAttachmentLoadOp)0;
		}

		static VkImageLayout GetAttachmentImageLayout(ImageFormat format)
		{
			if (Image::IsColorFormat(format))
				return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			if (Image::IsDepthFormat(format) || Image::IsStencilFormat(format))
				return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			ATN_CORE_ASSERT(false);
			return (VkImageLayout)0;
		}

		static VkClearValue GetClearValue(const FramebufferAttachmentInfo& info)
		{
			VkClearValue result = {};

			if (Image::IsColorFormat(info.Format))
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

		std::vector<VkAttachmentDescription> attachments;
		std::vector<VkAttachmentReference> colorAttachmentRefs;
		std::vector<VkSubpassDependency> dependencies;

		bool hasDepthStencil = false;
		VkAttachmentReference depthStencilAttachmentRef = {};

		for (const auto& attachment : m_Info.Output->GetInfo().Attachments)
		{
			VkAttachmentDescription attachmentDesc = {};
			attachmentDesc.format = Vulkan::GetFormat(attachment.Format);
			attachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;

			VkAttachmentLoadOp loadOp = Vulkan::GetLoadOp(m_Info.LoadOp);
			VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE;

			attachmentDesc.loadOp = loadOp;
			attachmentDesc.storeOp = storeOp;
			attachmentDesc.stencilLoadOp = loadOp;
			attachmentDesc.stencilStoreOp = storeOp;

			attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachmentDesc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			attachments.push_back(attachmentDesc);

			if (Image::IsColorFormat(attachment.Format))
			{
				VkAttachmentReference colorAttachmentRef;
				colorAttachmentRef.attachment = attachments.size() - 1;
				colorAttachmentRef.layout = Vulkan::GetAttachmentImageLayout(attachment.Format);

				colorAttachmentRefs.push_back(colorAttachmentRef);
			}
			else
			{
				depthStencilAttachmentRef.attachment = attachments.size() - 1;
				depthStencilAttachmentRef.layout = Vulkan::GetAttachmentImageLayout(attachment.Format);
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
		renderPassInfo.dependencyCount = dependencies.size();
		renderPassInfo.pDependencies = dependencies.data();

		VK_CHECK(vkCreateRenderPass(VulkanContext::GetLogicalDevice(), &renderPassInfo, nullptr, &m_VulkanRenderPass));
		Vulkan::SetObjectDebugName(m_VulkanRenderPass, VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT, m_Info.Name);

		m_Info.Output.As<VulkanFramebuffer>()->Bake(m_VulkanRenderPass);
	}

	VulkanRenderPass::~VulkanRenderPass()
	{
		Renderer::SubmitResourceFree([renderPass = m_VulkanRenderPass]()
		{
			vkDestroyRenderPass(VulkanContext::GetLogicalDevice(), renderPass, nullptr);
		});
	}

	void VulkanRenderPass::Begin(const Ref<RenderCommandBuffer>& commandBuffer)
	{
		Renderer::Submit([this, commandBuffer = commandBuffer]()
		{
			VkCommandBuffer vkcmdBuf = commandBuffer.As<VulkanRenderCommandBuffer>()->GetVulkanCommandBuffer();
			uint32 width = m_Info.Output->GetInfo().Width;
			uint32 height = m_Info.Output->GetInfo().Height;

			VkRenderPassBeginInfo renderPassBeginInfo = {};
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.renderPass = m_VulkanRenderPass;
			renderPassBeginInfo.framebuffer = m_Info.Output.As<VulkanFramebuffer>()->GetVulkanFramebuffer();
			renderPassBeginInfo.renderArea.offset = { 0, 0 };
			renderPassBeginInfo.renderArea.extent = { width , height };

			std::vector<VkClearValue> clearValues;
			if (m_Info.LoadOp == RenderPassLoadOp::CLEAR)
			{
				renderPassBeginInfo.clearValueCount = m_Info.Output->GetInfo().Attachments.size();
				clearValues.reserve(renderPassBeginInfo.clearValueCount);

				for (const auto& attachment : m_Info.Output->GetInfo().Attachments)
				{
					VkClearValue clearValue = Vulkan::GetClearValue(attachment);
					clearValues.push_back(clearValue);
				}

				renderPassBeginInfo.pClearValues = clearValues.data();
			}

			vkCmdBeginRenderPass(vkcmdBuf, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		});
	}

	void VulkanRenderPass::End(const Ref<RenderCommandBuffer>& commandBuffer)
	{
		Renderer::Submit([this, commandBuffer = commandBuffer]()
		{
			VkCommandBuffer vkcmdBuf = commandBuffer.As<VulkanRenderCommandBuffer>()->GetVulkanCommandBuffer();
			vkCmdEndRenderPass(vkcmdBuf);
		});
	}
}
