#include "VulkanTracyZone.h"

#include "Athena/Renderer/RHI/Vulkan/VulkanRHI.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanCommandContext.h"


namespace Athena
{
	VulkanTracyZone::VulkanTracyZone(const Ref<CommandContext>& cmdContext, uint32 line, std::string_view fileName, std::string_view functionName, const String& name)
#if ATN_ENABLE_TRACY_GPU
		: m_ScopedZone(
			cmdContext.As<VulkanCommandContext>()->GetTracyContext(),
			line,
			fileName.data(),
			fileName.size(),
			functionName.data(),
			functionName.size(),
			name.data(),
			name.size(),
			cmdContext.As<VulkanCommandContext>()->GetCommandBuffer()->CmdBuffer,
			true
		)
#endif
	{

	}

	VulkanTracyZone::~VulkanTracyZone()
	{
		// Zone goes out of scope
	}
}
