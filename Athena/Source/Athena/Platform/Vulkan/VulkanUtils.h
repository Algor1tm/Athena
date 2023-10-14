#include "Athena/Core/Core.h"
#include "Athena/Renderer/Shader.h"

#include <vulkan/vulkan.h>


namespace Athena::VulkanUtils
{
	static VkShaderStageFlagBits GetShaderStage(ShaderType type)
	{
		switch (type)
		{
		case ShaderType::VERTEX_SHADER: return VK_SHADER_STAGE_VERTEX_BIT;
		case ShaderType::FRAGMENT_SHADER: return VK_SHADER_STAGE_FRAGMENT_BIT;
		case ShaderType::GEOMETRY_SHADER: return VK_SHADER_STAGE_GEOMETRY_BIT;
		case ShaderType::COMPUTE_SHADER: return VK_SHADER_STAGE_COMPUTE_BIT;
		}

		ATN_CORE_ASSERT(false);
		return (VkShaderStageFlagBits)0;
	}
}
