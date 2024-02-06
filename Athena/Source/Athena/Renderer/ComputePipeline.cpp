#include "ComputePipeline.h"

#include "Athena/Renderer/Renderer.h"
#include "Athena/Platform/Vulkan/VulkanComputePipeline.h"


namespace Athena
{
	Ref<ComputePipeline> ComputePipeline::Create(const ComputePipelineCreateInfo& info)
	{
		switch (Renderer::GetAPI())
		{
		case Renderer::API::Vulkan: return Ref<VulkanComputePipeline>::Create(info);
		case Renderer::API::None: return nullptr;
		}

		return nullptr;
	}
}
