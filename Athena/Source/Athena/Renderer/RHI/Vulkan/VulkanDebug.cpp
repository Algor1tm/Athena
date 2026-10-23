#include "VulkanDebug.h"

#include "Athena/Renderer/RHI/Vulkan/VulkanRHI.h"
#include "Athena/Math/Random.h"


namespace Athena::Vulkan
{
#if VULKAN_ENABLE_DEBUG_INFO
    static VkDebugUtilsLabelEXT InitDebugUtilsLabel(const String& name, const IntegerColor& color)
    {
        LinearColor linearColor = color;

        if (linearColor == LinearColor::Black)
        {
            linearColor.r = Math::Random::Float(0.2f, 1.f);
            linearColor.g = Math::Random::Float(0.2f, 1.f);
            linearColor.b = Math::Random::Float(0.2f, 1.f);
            linearColor.a = 1.f;
        }

        VkDebugUtilsLabelEXT labelInfo = {};
        labelInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
        labelInfo.pLabelName = name.c_str();
        labelInfo.color[0] = linearColor.r;
        labelInfo.color[1] = linearColor.g;
        labelInfo.color[2] = linearColor.b;
        labelInfo.color[3] = linearColor.a;

        return labelInfo;
    }

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

    void BeginDebugLabel(VkCommandBuffer cmdBuffer, const String& name, const IntegerColor& color)
    {
        if (!VulkanRHI::Get()->GetExtensionsInfo().DebugUtilsEXTSupported)
            return;

        VkDebugUtilsLabelEXT labelInfo = InitDebugUtilsLabel(name, color);

        PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT = VulkanRHI::Get()->GetExtensionsInfo().vkCmdBeginDebugUtilsLabelEXT;
        vkCmdBeginDebugUtilsLabelEXT(cmdBuffer, &labelInfo);
    }

    void EndDebugLabel(VkCommandBuffer cmdBuffer)
    {
        if (!VulkanRHI::Get()->GetExtensionsInfo().DebugUtilsEXTSupported)
            return;

        PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT = VulkanRHI::Get()->GetExtensionsInfo().vkCmdEndDebugUtilsLabelEXT;
        vkCmdEndDebugUtilsLabelEXT(cmdBuffer);
    }

    void InsertDebugLabel(VkCommandBuffer cmdBuffer, const String& name, const IntegerColor& color)
    {
        if (!VulkanRHI::Get()->GetExtensionsInfo().DebugUtilsEXTSupported)
            return;

        VkDebugUtilsLabelEXT labelInfo = InitDebugUtilsLabel(name, color);

        PFN_vkCmdInsertDebugUtilsLabelEXT vkCmdInsertDebugUtilsLabelEXT = VulkanRHI::Get()->GetExtensionsInfo().vkCmdInsertDebugUtilsLabelEXT;
        vkCmdInsertDebugUtilsLabelEXT(cmdBuffer, &labelInfo);
    }
#endif
}
