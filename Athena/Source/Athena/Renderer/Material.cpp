#include "Material.h"

#include "Athena/Renderer/Renderer.h"
#include "Athena/Platform/Vulkan/VulkanMaterial.h"


namespace Athena
{
	Ref<Material> Material::Create(const Ref<Shader>& shader)
	{
		switch (Renderer::GetAPI())
		{
		case Renderer::API::Vulkan: return Ref<VulkanMaterial>::Create(shader);
		case Renderer::API::None: return nullptr;
		}

		return nullptr;
	}
}
