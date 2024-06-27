#include "ComputePipeline.h"

#include "Athena/Renderer/Renderer.h"
#include "Athena/Platform/Vulkan/VulkanComputePipeline.h"


namespace Athena
{
	Ref<ComputePipeline> ComputePipeline::Create(const Ref<Shader>& shader)
	{
		switch (Renderer::GetAPI())
		{
		case Renderer::API::Vulkan: return Ref<VulkanComputePipeline>::Create(shader);
		case Renderer::API::None: return nullptr;
		}

		return nullptr;
	}
}
