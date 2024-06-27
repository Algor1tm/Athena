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

	void RenderPass::SetOutput(const RenderTarget& target)
	{
		ATN_CORE_ASSERT(target.Texture);
		m_Outputs.push_back(target);
	}

	Ref<Texture2D> RenderPass::GetOutput(const String& name) const
	{
		for (const auto& target : m_Outputs)
		{
			if (target.Texture->GetName() == name)
			{
				return target.Texture;
			}
		}

		ATN_CORE_ASSERT(false);
		return nullptr;
	}

	Ref<Texture2D> RenderPass::GetOutput(uint32 index) const
	{
		if (index < m_Outputs.size())
			return m_Outputs[index].Texture;

		ATN_CORE_ASSERT(false);
		return nullptr;
	}

	Ref<Texture2D> RenderPass::GetDepthOutput() const
	{
		for (const auto& target : m_Outputs)
		{
			if (Texture::IsDepthFormat(target.Texture->GetFormat()))
			{
				return target.Texture;
			}
		}

		ATN_CORE_ASSERT(false);
		return nullptr;
	}

	uint32 RenderPass::GetColorTargetsCount() const
	{
		uint32 count = 0;
		for (const auto& target : m_Outputs)
		{
			if (Texture::IsColorFormat(target.Texture->GetFormat()))
			{
				count++;
			}
		}

		return count;
	}

	bool RenderPass::HasDepthRenderTarget() const
	{
		for (const auto& target : m_Outputs)
		{
			if (Texture::IsDepthFormat(target.Texture->GetFormat()))
			{
				return true;
			}
		}

		return false;
	}
}