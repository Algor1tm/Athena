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
}