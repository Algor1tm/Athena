#include "GPUProfiler.h"

#include "Athena/Platform/Vulkan/VulkanProfiler.h"


namespace Athena
{
	Ref<GPUProfiler> GPUProfiler::Create(const GPUProfilerCreateInfo& info)
	{
		switch (Renderer::GetAPI())
		{
		case Renderer::API::Vulkan: return Ref<VulkanProfiler>::Create(info);
		case Renderer::API::None: return nullptr;
		}

		return nullptr;
	}
}