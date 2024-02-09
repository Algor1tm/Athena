#include "VulkanRenderPass.h"

#include "Athena/Platform/Vulkan/VulkanUtils.h"
#include "Athena/Platform/Vulkan/VulkanTexture2D.h"
#include "Athena/Platform/Vulkan/VulkanImage.h"
#include "Athena/Platform/Vulkan/VulkanRenderCommandBuffer.h"


namespace Athena
{
	namespace Vulkan
	{
		static VkAttachmentLoadOp GetLoadOp(AttachmentLoadOp loadOp)
		{
			switch (loadOp)
			{
			case AttachmentLoadOp::DONT_CARE: return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			case AttachmentLoadOp::CLEAR: return VK_ATTACHMENT_LOAD_OP_CLEAR;
			case AttachmentLoadOp::LOAD: return VK_ATTACHMENT_LOAD_OP_LOAD;
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

		static VkClearValue GetClearValue(const AttachmentInfo& info)
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

		m_DepthAttachment = nullptr;

		std::vector<VkAttachmentDescription> attachments;
		std::vector<VkAttachmentReference> colorAttachmentRefs;

		bool hasDepthStencil = false;
		VkAttachmentReference depthStencilAttachmentRef = {};

		for (const auto& attachment : m_Info.Attachments)
		{
			VkAttachmentDescription attachmentDesc = {};
			attachmentDesc.format = Vulkan::GetFormat(attachment.Format);
			attachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;

			VkAttachmentLoadOp loadOp = Vulkan::GetLoadOp(attachment.LoadOp);
			VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE;

			attachmentDesc.loadOp = loadOp;
			attachmentDesc.storeOp = storeOp;
			attachmentDesc.stencilLoadOp = loadOp;
			attachmentDesc.stencilStoreOp = storeOp;

			attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachmentDesc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			attachments.push_back(attachmentDesc);

			uint32 usage = ImageUsage::ATTACHMENT | ImageUsage::SAMPLED | ImageUsage::TRANSFER_SRC;

			Texture2DCreateInfo attachmentInfo = {};
			attachmentInfo.Format = attachment.Format;
			attachmentInfo.Usage = (ImageUsage)usage;
			attachmentInfo.Name = attachment.Name;
			attachmentInfo.Width = m_Info.Width;
			attachmentInfo.Height = m_Info.Height;
			attachmentInfo.Layers = 1;
			attachmentInfo.MipLevels = 1;
			attachmentInfo.SamplerInfo.MinFilter = TextureFilter::LINEAR;
			attachmentInfo.SamplerInfo.MagFilter = TextureFilter::LINEAR;
			attachmentInfo.SamplerInfo.Wrap = TextureWrap::REPEAT;

			if (Image::IsColorFormat(attachment.Format))
			{
				VkAttachmentReference colorAttachmentRef;
				colorAttachmentRef.attachment = attachments.size() - 1;
				colorAttachmentRef.layout = Vulkan::GetAttachmentImageLayout(attachment.Format);

				colorAttachmentRefs.push_back(colorAttachmentRef);

				m_ColorAttachments.push_back(Texture2D::Create(attachmentInfo));
			}
			else
			{
				depthStencilAttachmentRef.attachment = attachments.size() - 1;
				depthStencilAttachmentRef.layout = Vulkan::GetAttachmentImageLayout(attachment.Format);
				hasDepthStencil = true;

				ATN_CORE_VERIFY(m_DepthAttachment == nullptr, "Max 1 depth attachment in framebuffer!");
				m_DepthAttachment = Texture2D::Create(attachmentInfo);
			}
		}
		for (const auto& attachment : m_Info.ExistingImages)
		{
			ImageFormat format = attachment->GetInfo().Format;
			VkAttachmentDescription attachmentDesc = {};
			attachmentDesc.format = Vulkan::GetFormat(format);
			attachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;

			VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
			VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE;

			attachmentDesc.loadOp = loadOp;
			attachmentDesc.storeOp = storeOp;
			attachmentDesc.stencilLoadOp = loadOp;
			attachmentDesc.stencilStoreOp = storeOp;

			attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			attachmentDesc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			attachments.push_back(attachmentDesc);

			if (Image::IsColorFormat(format))
			{
				VkAttachmentReference colorAttachmentRef;
				colorAttachmentRef.attachment = attachments.size() - 1;
				colorAttachmentRef.layout = Vulkan::GetAttachmentImageLayout(format);

				colorAttachmentRefs.push_back(colorAttachmentRef);
			}
			else
			{
				depthStencilAttachmentRef.attachment = attachments.size() - 1;
				depthStencilAttachmentRef.layout = Vulkan::GetAttachmentImageLayout(format);
				hasDepthStencil = true;
			}
		}

		std::vector<VkSubpassDependency> dependencies;

		if (m_Info.InputPass)
		{
			VkSubpassDependency dependency = {};
			dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
			dependency.dstSubpass = 0;
			dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependency.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
			dependencies.push_back(dependency);
		}

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

		for (const auto& attachment : m_Info.Attachments)
		{
			if (attachment.LoadOp == AttachmentLoadOp::CLEAR)
			{
				VkClearValue clearValue = Vulkan::GetClearValue(attachment);
				m_ClearColors.push_back(clearValue);
			}
		}

		m_Info.Width = 0;
		m_Info.Height = 0;
		Resize(info.Width, info.Height);
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
		Renderer::Submit([this, commandBuffer = commandBuffer]()
		{
			VkCommandBuffer vkcmdBuf = commandBuffer.As<VulkanRenderCommandBuffer>()->GetVulkanCommandBuffer();
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

		for (auto& attachment : m_ColorAttachments)
			attachment->Resize(width, height);

		if (m_DepthAttachment)
			m_DepthAttachment->Resize(width, height);

		Renderer::Submit([this]()
		{
			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = m_VulkanRenderPass;
			framebufferInfo.attachmentCount = m_Info.Attachments.size();
			framebufferInfo.width = m_Info.Width;
			framebufferInfo.height = m_Info.Height;
			framebufferInfo.layers = 1;

			std::vector<VkImageView> attachments;

			for (auto& colorAttachment : m_ColorAttachments)
			{
				VkImageView attachment = colorAttachment.As<VulkanTexture2D>()->GetVulkanImageView();
				attachments.push_back(attachment);
			}

			for (auto& image : m_Info.ExistingImages)
			{
				VkImageView attachment = image.As<VulkanImage>()->GetVulkanImageView();
				attachments.push_back(attachment);
			}

			if (m_DepthAttachment != nullptr)
			{
				VkImageView attachment = m_DepthAttachment.As<VulkanTexture2D>()->GetVulkanImageView();
				attachments.push_back(attachment);
			}

			framebufferInfo.attachmentCount = attachments.size();
			framebufferInfo.pAttachments = attachments.data();

			VK_CHECK(vkCreateFramebuffer(VulkanContext::GetLogicalDevice(), &framebufferInfo, nullptr, &m_VulkanFramebuffer));
			Vulkan::SetObjectDebugName(m_VulkanFramebuffer, VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT, std::format("{}_FB", m_Info.Name));
		});
	}

	Ref<Texture2D> VulkanRenderPass::GetOutput(uint32 attachmentIndex) const
	{
		ATN_CORE_ASSERT(m_ColorAttachments.size() > attachmentIndex);
		return m_ColorAttachments[attachmentIndex];
	}

	Ref<Texture2D> VulkanRenderPass::GetDepthOutput() const
	{
		ATN_CORE_ASSERT(m_DepthAttachment);
		return m_DepthAttachment;
	}

	uint32 VulkanRenderPass::GetColorAttachmentCount() const
	{
		uint32 count = m_ColorAttachments.size();
		for (const auto& image : m_Info.ExistingImages)
		{
			if (Image::IsColorFormat(image->GetInfo().Format))
				count++;
		}

		return count;
	}

	bool VulkanRenderPass::HasDepthAttachment() const
	{
		if (m_DepthAttachment)
			return true;

		for (const auto& image : m_Info.ExistingImages)
		{
			if (!Image::IsColorFormat(image->GetInfo().Format))
				return true;
		}

		return false;
	}
}
