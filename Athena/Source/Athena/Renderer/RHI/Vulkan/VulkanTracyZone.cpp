#include "VulkanTracyZone.h"

#include "Athena/Renderer/RHI/Vulkan/VulkanRHI.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanCommandList.h"


namespace Athena
{
	VulkanTracyZone::VulkanTracyZone(const Ref<CommandList>& cmdList, uint32 line, std::string_view fileName, std::string_view functionName, const String& name)
#if ATN_ENABLE_TRACY_GPU
		: m_ScopedZone(
			VulkanRHI::Get()->GetCmdBufferManager()->GetTracyContext(),
			line,
			fileName.data(),
			fileName.size(),
			functionName.data(),
			functionName.size(),
			name.data(),
			name.size(),
			cmdList.As<VulkanCommandList>()->GetCommandBuffer()->CommandBuffer,
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
