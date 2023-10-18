#include "Image.h"

#include "Athena/Renderer/Renderer.h"
#include "Athena/Platform/Vulkan/VulkanImage.h"


namespace Athena
{
	Ref<Image> Image::Create(const ImageCreateInfo& info)
	{
		switch (Renderer::GetAPI())
		{
		case Renderer::API::Vulkan: return Ref<VulkanImage>::Create(info);
		case Renderer::API::None: return nullptr;
		}

		return nullptr;
	}
}
