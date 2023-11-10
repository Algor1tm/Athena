#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Material.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	class VulkanMaterial : public Material
	{
	public:
		VulkanMaterial(const Ref<Shader>& shader);
		~VulkanMaterial();

		virtual void RT_UpdateForRendering() override;

		VkDescriptorSetLayout GetDescriptorSetLayout() { return m_DescriptorSetLayout; }

	private:
		VkDescriptorSetLayout m_DescriptorSetLayout;
	};
}
