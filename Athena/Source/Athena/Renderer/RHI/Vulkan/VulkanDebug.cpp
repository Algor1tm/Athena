#include "VulkanDebug.h"

#include "Athena/Renderer/RHI/Vulkan/VulkanRHI.h"

namespace Athena::Vulkan
{
    void SetDebugName(void* objectHandle, VkObjectType type, const String& name)
    {
        if (!VulkanRHI::Get()->GetExtensionsInfo().DebugUtilsEXTSupported)
            return;

        VkDebugUtilsObjectNameInfoEXT nameInfo = {};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = type;
        nameInfo.objectHandle = (uint64)objectHandle;
        nameInfo.pObjectName = name.c_str();

        String subName;
        if (name.size() >= VULKAN_MAX_DEBUG_NAME_LENGTH)
        {
            subName = name.substr(0, VULKAN_MAX_DEBUG_NAME_LENGTH);
            nameInfo.pObjectName = subName.c_str();
        }

        PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT = VulkanRHI::Get()->GetExtensionsInfo().vkSetDebugUtilsObjectNameEXT;
        vkSetDebugUtilsObjectNameEXT(VulkanRHI::Get()->GetDevice()->GetLogicalDevice(), &nameInfo);
    }
}
