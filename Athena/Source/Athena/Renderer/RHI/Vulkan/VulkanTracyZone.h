#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/CommandList.h"

#include <vulkan/vulkan.h>
#include <tracy/TracyVulkan.hpp>


// Macros for vulkan backend
#if ATN_ENABLE_TRACY_GPU
	#define TRACY_VULKAN_ZONE(cmdBuffer, name) TracyVkZone(VulkanRHI::Get()->GetCmdBufferManager()->GetTracyContext(), cmdBuffer, name)
#else
	#define TRACY_VULKAN_ZONE(cmdBuffer, name)
#endif


namespace Athena
{
	class VulkanTracyZone: public TracyGPUZone
	{
	public:
		VulkanTracyZone(const Ref<CommandList>& list, uint32 line, std::string_view fileName, std::string_view functionName, const String& name);
		virtual ~VulkanTracyZone();

	private:
#if ATN_ENABLE_TRACY_GPU
		tracy::VkCtxScope m_ScopedZone;
#endif
	};
}
