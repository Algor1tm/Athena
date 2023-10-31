#include "GPUProfiler.h"

#include "Athena/Platform/Vulkan/VulkanProfiler.h"


namespace Athena
{
	Ref<GPUProfiler> GPUProfiler::Create()
	{
		switch (Renderer::GetAPI())
		{
		case Renderer::API::Vulkan: return Ref<VulkanProfiler>::Create();
		case Renderer::API::None: return nullptr;
		}

		return nullptr;
	}
}