#include "RenderPass.h"

#include "Athena/Renderer/Renderer.h"
#include "Athena/Platform/Vulkan/VulkanRenderPass.h"


namespace Athena
{
	Ref<RenderPass> RenderPass::Create(const RenderPassCreateInfo& info)
	{
		switch (Renderer::GetAPI())
		{
		case Renderer::API::Vulkan: return Ref<VulkanRenderPass>::Create(info);
		case Renderer::API::None: return nullptr;
		}

		return nullptr;
	}

	void RenderPass::SetOutput(const RenderPassAttachment& attachment)
	{
		ATN_CORE_ASSERT(attachment.Texture);
		m_Outputs.push_back(attachment);
	}

	Ref<Texture2D> RenderPass::GetOutput(const String& name) const
	{
		for (const auto& attachment : m_Outputs)
		{
			if (attachment.Texture->GetName() == name)
			{
				return attachment.Texture;
			}
		}

		return nullptr;
	}

	Ref<Texture2D> RenderPass::GetDepthOutput() const
	{
		for (const auto& attachment : m_Outputs)
		{
			if (Image::IsDepthFormat(attachment.Texture->GetFormat()))
			{
				return attachment.Texture;
			}
		}

		return nullptr;
	}

	uint32 RenderPass::GetColorAttachmentCount() const
	{
		uint32 count = 0;
		for (const auto& attachment : m_Outputs)
		{
			if (Image::IsColorFormat(attachment.Texture->GetFormat()))
			{
				count++;
			}
		}

		return count;
	}

	bool RenderPass::HasDepthAttachment() const
	{
		for (const auto& attachment : m_Outputs)
		{
			if (Image::IsDepthFormat(attachment.Texture->GetFormat()))
			{
				return true;
			}
		}

		return false;
	}
}