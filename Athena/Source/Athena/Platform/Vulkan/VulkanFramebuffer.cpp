#include "VulkanFramebuffer.h"

#include "Athena/Platform/Vulkan/VulkanUtils.h"
#include "Athena/Platform/Vulkan/VulkanTexture2D.h"


namespace Athena
{
	VulkanFramebuffer::VulkanFramebuffer(const FramebufferCreateInfo& info)
	{
		m_Info = info;

		bool hasColorFormat = false;
		bool hasDepthFormat = false;
		for (auto attachment : m_Info.Attachments)
		{
			if (Texture::IsColorFormat(attachment.Format))
				hasColorFormat = true;
			else
				hasDepthFormat = true;
		}

		if (hasDepthFormat)
			m_DepthAttachmentSet.resize(Renderer::GetFramesInFlight());

		if (hasColorFormat)
			m_ColorAttachmentsSet.resize(Renderer::GetFramesInFlight());

		for (uint32 frameIndex = 0; frameIndex < Renderer::GetFramesInFlight(); ++frameIndex)
		{
			for (const auto& attachment : m_Info.Attachments)
			{
				TextureCreateInfo attachmentInfo = {};
				attachmentInfo.Width = m_Info.Width;
				attachmentInfo.Height = m_Info.Height;
				attachmentInfo.GenerateMipMap = false;
				attachmentInfo.Format = attachment.Format;
				attachmentInfo.Usage = TextureUsage::ATTACHMENT;
				attachmentInfo.GenerateSampler = true;
				attachmentInfo.SamplerInfo.MinFilter = TextureFilter::LINEAR;
				attachmentInfo.SamplerInfo.MagFilter = TextureFilter::LINEAR;
				attachmentInfo.SamplerInfo.Wrap = TextureWrap::REPEAT;

				if (Texture::IsColorFormat(attachment.Format))
				{
					m_ColorAttachmentsSet[frameIndex].push_back(Texture2D::Create(attachmentInfo));
				}
				else
				{
					ATN_CORE_ASSERT(m_DepthAttachmentSet[frameIndex] == nullptr);
					m_DepthAttachmentSet[frameIndex] = Texture2D::Create(attachmentInfo);
				}
			}
		}
	}

	VulkanFramebuffer::~VulkanFramebuffer()
	{
		CleanUpFramebufferSet();
	}

	void VulkanFramebuffer::Resize(uint32 width, uint32 height)
	{
		m_Info.Width = width;
		m_Info.Height = height;

		CleanUpFramebufferSet();

		for (uint32 frameIndex = 0; frameIndex < Renderer::GetFramesInFlight(); ++frameIndex)
		{
			for (auto& attachment : m_ColorAttachmentsSet[frameIndex])
				attachment->Resize(width, height);

			if (m_DepthAttachmentSet.size() != 0)
				m_DepthAttachmentSet[frameIndex]->Resize(width, height);
		}

		Renderer::Submit([this]()
		{
			ResizeFramebufferSet();
		});
	}

	const Ref<Texture2D>& VulkanFramebuffer::GetColorAttachment(uint32 index) const
	{
		ATN_CORE_ASSERT(m_ColorAttachmentsSet[Renderer::GetCurrentFrameIndex()].size() > index);

		return m_ColorAttachmentsSet[Renderer::GetCurrentFrameIndex()][index];
	}

	const Ref<Texture2D>& VulkanFramebuffer::GetDepthAttachment() const
	{
		ATN_CORE_ASSERT(m_DepthAttachmentSet.size() != 0);

		return m_DepthAttachmentSet[Renderer::GetCurrentFrameIndex()];
	}

	VkFramebuffer VulkanFramebuffer::GetVulkanFramebuffer() const 
	{
		return m_VulkanFramebufferSet[Renderer::GetCurrentFrameIndex()]; 
	}

	void VulkanFramebuffer::RT_BakeFramebuffer(VkRenderPass renderPass)
	{
		m_VulkanRenderPass = renderPass;

		m_VulkanFramebufferSet.resize(Renderer::GetFramesInFlight());
		ResizeFramebufferSet();
	}

	void VulkanFramebuffer::CleanUpFramebufferSet()
	{
		if (m_VulkanFramebufferSet.size() > 0)
		{
			Renderer::SubmitResourceFree([framebufferSet = m_VulkanFramebufferSet]()
			{
				for (auto framebuffer : framebufferSet)
				{
					vkDestroyFramebuffer(VulkanContext::GetLogicalDevice(), framebuffer, nullptr);
				}
			});
		}
	}

	void VulkanFramebuffer::ResizeFramebufferSet()
	{
		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_VulkanRenderPass;
		framebufferInfo.attachmentCount = m_Info.Attachments.size();
		framebufferInfo.width = m_Info.Width;
		framebufferInfo.height = m_Info.Height;
		framebufferInfo.layers = 1;

		for (uint32 frameIndex = 0; frameIndex < Renderer::GetFramesInFlight(); ++frameIndex)
		{
			std::vector<VkImageView> attachments;

			if (m_ColorAttachmentsSet.size() > 0)
			{
				for (auto& colorAttachment : m_ColorAttachmentsSet[frameIndex])
				{
					VkImageView attachment = colorAttachment.As<VulkanTexture2D>()->GetVulkanImageView();
					attachments.push_back(attachment);
				}
			}

			if (m_DepthAttachmentSet.size() > 0)
			{
				VkImageView attachment = m_DepthAttachmentSet[frameIndex].As<VulkanTexture2D>()->GetVulkanImageView();
				attachments.push_back(attachment);
			}

			framebufferInfo.pAttachments = attachments.data();

			VK_CHECK(vkCreateFramebuffer(VulkanContext::GetLogicalDevice(), &framebufferInfo, nullptr, &m_VulkanFramebufferSet[frameIndex]));
		}
	}
}
