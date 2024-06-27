#pragma once

#include "Athena/Renderer/RendererAPI.h"
#include "Athena/Platform/Vulkan/VulkanRenderer.h"


namespace Athena
{
	Ref<RendererAPI> RendererAPI::Create(Renderer::API api)
	{
		switch (Renderer::GetAPI())
		{
		case Renderer::API::Vulkan: return Ref<VulkanRenderer>::Create();
		case Renderer::API::None: return nullptr;
		}

		return nullptr;
	}
}
