#include "ComputePass.h"

#include "Athena/Renderer/Renderer.h"
#include "Athena/Platform/Vulkan/VulkanComputePass.h"


namespace Athena
{
	Ref<ComputePass> ComputePass::Create(const ComputePassCreateInfo& info)
	{
		switch (Renderer::GetAPI())
		{
		case Renderer::API::Vulkan: return Ref<VulkanComputePass>::Create(info);
		case Renderer::API::None: return nullptr;
		}

		return nullptr;
	}

	void ComputePass::SetOutput(const Ref<Texture2D>& texture)
	{
		ATN_CORE_ASSERT(texture);
		m_Texture2DOutputs.push_back(texture);
	}

	void ComputePass::SetOutput(const Ref<TextureCube>& texture)
	{
		ATN_CORE_ASSERT(texture);
		m_TextureCubeOutputs.push_back(texture);
	}

	Ref<Texture2D> ComputePass::GetOutput(const String& name)
	{
		for (const auto& output : m_Texture2DOutputs)
		{
			if (output->GetName() == name)
			{
				return output;
			}
		}

		ATN_CORE_ASSERT(false);
		return nullptr;
	}
}