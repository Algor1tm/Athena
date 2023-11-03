#include "GPUProfiler.h"

#include "Athena/Platform/Vulkan/VulkanProfiler.h"


namespace Athena
{
	Ref<GPUProfiler> GPUProfiler::Create(uint32 maxTimestamps, uint32 maxPipelineQueries)
	{
		switch (Renderer::GetAPI())
		{
		case Renderer::API::Vulkan: return Ref<VulkanProfiler>::Create(maxTimestamps, maxPipelineQueries);
		case Renderer::API::None: return nullptr;
		}

		return nullptr;
	}
}