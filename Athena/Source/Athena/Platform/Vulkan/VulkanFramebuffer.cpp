#include "VulkanFramebuffer.h"

#include "Athena/Platform/Vulkan/VulkanUtils.h"
#include "Athena/Platform/Vulkan/VulkanTexture2D.h"


namespace Athena
{
	VulkanFramebuffer::VulkanFramebuffer(const FramebufferCreateInfo& info)
	{
		m_Info = info;

		m_DepthAttachment = nullptr;
		m_VulkanFramebuffer = VK_NULL_HANDLE;
		m_VulkanRenderPass = VK_NULL_HANDLE;

		for (const auto& attachment : m_Info.Attachments)
		{
			TextureCreateInfo attachmentInfo = {};
			attachmentInfo.Format = attachment.Format;
			attachmentInfo.Usage = ImageUsage::ATTACHMENT;
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
				m_ColorAttachments.push_back(Texture2D::Create(attachmentInfo));
			}
			else
			{
				ATN_CORE_VERIFY(m_DepthAttachment == nullptr, "Max 1 depth attachment in framebuffer!");
				m_DepthAttachment = Texture2D::Create(attachmentInfo);
			}
		}
	}

	VulkanFramebuffer::~VulkanFramebuffer()
	{
		CleanUpFramebuffer();
	}

	void VulkanFramebuffer::Resize(uint32 width, uint32 height)
	{
		m_Info.Width = width;
		m_Info.Height = height;

		CleanUpFramebuffer();

		for (auto& attachment : m_ColorAttachments)
			attachment->Resize(width, height);

		if (HasDepthAttachment())
			m_DepthAttachment->Resize(width, height);

		ResizeFramebuffer();
	}

	const Ref<Texture2D>& VulkanFramebuffer::GetColorAttachment(uint32 index) const
	{
		ATN_CORE_ASSERT(m_ColorAttachments.size() > index);

		return m_ColorAttachments[index];
	}

	const Ref<Texture2D>& VulkanFramebuffer::GetDepthAttachment() const
	{
		ATN_CORE_ASSERT(HasDepthAttachment());

		return m_DepthAttachment;
	}

	uint32 VulkanFramebuffer::GetColorAttachmentCount() const
	{
		return m_ColorAttachments.size();
	}

	bool VulkanFramebuffer::HasDepthAttachment() const
	{
		return m_DepthAttachment != nullptr;
	}

	VkFramebuffer VulkanFramebuffer::GetVulkanFramebuffer() const 
	{
		return m_VulkanFramebuffer; 
	}

	void VulkanFramebuffer::Bake(VkRenderPass renderPass)
	{
		m_VulkanRenderPass = renderPass;

		ResizeFramebuffer();
	}

	void VulkanFramebuffer::CleanUpFramebuffer()
	{
		if (m_VulkanFramebuffer)
		{
			Renderer::SubmitResourceFree([framebuffer = m_VulkanFramebuffer]()
			{
				vkDestroyFramebuffer(VulkanContext::GetLogicalDevice(), framebuffer, nullptr);
			});
		}
	}

	void VulkanFramebuffer::ResizeFramebuffer()
	{
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

			if (m_DepthAttachment != nullptr)
			{
				VkImageView attachment = m_DepthAttachment.As<VulkanTexture2D>()->GetVulkanImageView();
				attachments.push_back(attachment);
			}

			framebufferInfo.pAttachments = attachments.data();

			VK_CHECK(vkCreateFramebuffer(VulkanContext::GetLogicalDevice(), &framebufferInfo, nullptr, &m_VulkanFramebuffer));
			Vulkan::SetObjectName(m_VulkanFramebuffer, VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT, m_Info.Name);
		});
	}
}
