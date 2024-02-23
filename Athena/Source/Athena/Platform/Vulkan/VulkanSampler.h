#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Texture.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	class VulkanSampler : public Sampler
	{
	public:
		VulkanSampler(const TextureSamplerCreateInfo& info);
		~VulkanSampler();

		VkSampler GetVulkanSampler();
		const VkDescriptorImageInfo& GetVulkanDescriptorInfo();

	private:
		void Recreate();
		void CleanUp();

	private:
		VkSampler m_VulkanSampler;
		VkDescriptorImageInfo m_DescriptorInfo;
	};
}
