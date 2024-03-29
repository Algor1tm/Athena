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

	void ComputePass::SetOutput(const Ref<RenderResource>& resource)
	{
		ATN_CORE_ASSERT(resource);
		ATN_CORE_ASSERT(resource->GetResourceType() != RenderResourceType::UniformBuffer);
		m_Outputs.push_back(resource);
	}

	Ref<RenderResource> ComputePass::GetOutput(const String& name)
	{
		for (const auto& output : m_Outputs)
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