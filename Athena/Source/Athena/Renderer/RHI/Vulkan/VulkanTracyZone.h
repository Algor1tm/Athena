#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/CommandContext.h"

#include <vulkan/vulkan.h>

// Macros for vulkan backend
#if ATN_ENABLE_TRACY_GPU
	#include <tracy/TracyVulkan.hpp>

	#define TRACY_VULKAN_ZONE(cmdBuffer, name) TracyVkZone(cmdBuffer->Context->GetTracyContext(), cmdBuffer->CmdBuffer, name)
#else
	#define TRACY_VULKAN_ZONE(cmdBuffer, name)
#endif


namespace Athena
{
	class VulkanTracyZone: public TracyGPUZone
	{
	public:
		VulkanTracyZone(const Ref<CommandContext>& cmdContext, uint32 line, std::string_view fileName, std::string_view functionName, const String& name);
		virtual ~VulkanTracyZone();

	private:
#if ATN_ENABLE_TRACY_GPU
		tracy::VkCtxScope m_ScopedZone;
#endif
	};
}
