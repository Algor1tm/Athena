#include "VulkanDebug.h"

#include "Athena/Renderer/RHI/Vulkan/VulkanRHI.h"

namespace Athena::Vulkan
{
    void SetDebugName(void* object, VkDebugReportObjectTypeEXT type, const String& name)
    {
        VkDebugMarkerObjectNameInfoEXT nameInfo = {};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = type;
        nameInfo.object = (uint64)object;
        nameInfo.pObjectName = name.c_str();

        String subName;
        if (name.size() >= VULKAN_MAX_DEBUG_NAME_LENGTH)
        {
            subName = name.substr(0, VULKAN_MAX_DEBUG_NAME_LENGTH);
            nameInfo.pObjectName = subName.c_str();
        }

        PFN_vkDebugMarkerSetObjectNameEXT vkDebugMarkerSetObjectNameEXT = VulkanRHI::Get()->GetDevice()->GetExtensionsInfo().vkDebugMarkerSetObjectNameEXT;
        vkDebugMarkerSetObjectNameEXT(VulkanRHI::Get()->GetDevice()->GetLogicalDevice(), &nameInfo);
    }
}
