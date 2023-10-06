#include "SwapChain.h"

#include "Athena/Renderer/Renderer.h"

#include "Athena/Platform/Vulkan/VulkanSwapChain.h"


namespace Athena
{
	Ref<SwapChain> SwapChain::Create(void* windowHandle)
	{
		switch (Renderer::GetAPI())
		{
		case Renderer::API::Vulkan: return Ref<VulkanSwapChain>::Create(windowHandle);
		case Renderer::API::None: return nullptr;
		}

		return nullptr;
	}
}
