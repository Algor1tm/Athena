#include "Pipeline.h"

#include "Athena/Renderer/Renderer.h"
#include "Athena/Platform/Vulkan/VulkanPipeline.h"


namespace Athena
{
	Ref<Pipeline> Pipeline::Create(const PipelineCreateInfo& info)
	{
		switch (Renderer::GetAPI())
		{
		case Renderer::API::Vulkan: return Ref<VulkanPipeline>::Create(info);
		case Renderer::API::None: return nullptr;
		}

		return nullptr;
	}
}
