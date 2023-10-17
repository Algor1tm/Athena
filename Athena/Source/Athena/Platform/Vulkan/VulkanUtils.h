#include "Athena/Core/Core.h"
#include "Athena/Renderer/Shader.h"

#include <vulkan/vulkan.h>

#define DEFAULT_FENCE_TIMEOUT 100000000000


namespace Athena::VulkanUtils
{
	static VkShaderStageFlagBits GetShaderStage(ShaderStage stage)
	{
		switch (stage)
		{
		case ShaderStage::VERTEX_STAGE: return VK_SHADER_STAGE_VERTEX_BIT;
		case ShaderStage::FRAGMENT_STAGE: return VK_SHADER_STAGE_FRAGMENT_BIT;
		case ShaderStage::GEOMETRY_STAGE: return VK_SHADER_STAGE_GEOMETRY_BIT;
		case ShaderStage::COMPUTE_STAGE: return VK_SHADER_STAGE_COMPUTE_BIT;
		}

		ATN_CORE_ASSERT(false);
		return (VkShaderStageFlagBits)0;
	}
}
