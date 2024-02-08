#include "Athena/Core/Core.h"

#include "Athena/Renderer/Shader.h"
#include "Athena/Renderer/GPUBuffers.h"
#include "Athena/Renderer/Texture.h"

#include "Athena/Platform/Vulkan/VulkanContext.h"

#include <vulkan/vulkan.h>

#define DEFAULT_FENCE_TIMEOUT 100000000000


namespace Athena::Vulkan
{
    // Copied from glfw internal
    inline const char* GetResultString(VkResult result)
    {
        switch (result)
        {
        case VK_SUCCESS:
            return "Success";
        case VK_NOT_READY:
            return "A fence or query has not yet completed";
        case VK_TIMEOUT:
            return "A wait operation has not completed in the specified time";
        case VK_EVENT_SET:
            return "An event is signaled";
        case VK_EVENT_RESET:
            return "An event is unsignaled";
        case VK_INCOMPLETE:
            return "A return array was too small for the result";
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            return "A host memory allocation has failed";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            return "A device memory allocation has failed";
        case VK_ERROR_INITIALIZATION_FAILED:
            return "Initialization of an object could not be completed for implementation-specific reasons";
        case VK_ERROR_DEVICE_LOST:
            return "The logical or physical device has been lost";
        case VK_ERROR_MEMORY_MAP_FAILED:
            return "Mapping of a memory object has failed";
        case VK_ERROR_LAYER_NOT_PRESENT:
            return "A requested layer is not present or could not be loaded";
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            return "A requested extension is not supported";
        case VK_ERROR_FEATURE_NOT_PRESENT:
            return "A requested feature is not supported";
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            return "The requested version of Vulkan is not supported by the driver or is otherwise incompatible";
        case VK_ERROR_TOO_MANY_OBJECTS:
            return "Too many objects of the type have already been created";
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
            return "A requested format is not supported on this device";
        case VK_ERROR_SURFACE_LOST_KHR:
            return "A surface is no longer available";
        case VK_SUBOPTIMAL_KHR:
            return "A swapchain no longer matches the surface properties exactly, but can still be used";
        case VK_ERROR_OUT_OF_DATE_KHR:
            return "A surface has changed in such a way that it is no longer compatible with the swapchain";
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
            return "The display used by a swapchain does not use the same presentable image layout";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
            return "The requested window is already connected to a VkSurfaceKHR, or to some other non-Vulkan API";
        case VK_ERROR_VALIDATION_FAILED_EXT:
            return "A validation layer found an error";
        default:
            return "UNKNOWN VULKAN ERROR";
        }
    }

    inline bool CheckResult(VkResult error)
    {
        if (error == 0)
            return true;

        const char* errorString = GetResultString(error);

        if (error > 0)
            ATN_CORE_ERROR_TAG("Vulkan", "VkResult = {}, Error: {}", (int)error, errorString);

        if (error < 0)
            ATN_CORE_FATAL_TAG("Vulkan", "VkResult = {}, Fatal Error: {}", (int)error, errorString);

        return false;
    }

#ifdef ATN_DEBUG
    #define VK_CHECK(expr) ATN_CORE_ASSERT(::Athena::Vulkan::CheckResult(expr))
#else
    #define VK_CHECK(expr) expr
#endif


    inline void SetObjectDebugName(void* object, VkDebugReportObjectTypeEXT type, const String& name)
    {
#ifdef ATN_DEBUG
        static PFN_vkDebugMarkerSetObjectNameEXT PFN_DebugMarkerSetObjectName = nullptr;

        if (PFN_DebugMarkerSetObjectName == nullptr)
        {
            PFN_DebugMarkerSetObjectName = (PFN_vkDebugMarkerSetObjectNameEXT)vkGetDeviceProcAddr(VulkanContext::GetLogicalDevice(), "vkDebugMarkerSetObjectNameEXT");
        }

        VkDebugMarkerObjectNameInfoEXT nameInfo = {};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = type;
        nameInfo.object = (uint64)object;
        nameInfo.pObjectName = name.c_str();

        PFN_DebugMarkerSetObjectName(VulkanContext::GetLogicalDevice(), &nameInfo);
#endif
    }

    inline VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType,
        uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
    {
        String message = String(pMessage);

        std::vector<String> objects;
        const std::string_view objectLabel = " Object ";
        const std::string_view handleLabel = " handle = ";
        const std::string_view nameLabel = " name = ";

        // Collect objects in array
        uint64 pos = message.find(objectLabel);
        while (pos != String::npos)
        {
            uint64 begin = pos + objectLabel.size() + 1;
            uint64 end = message.find(';', begin);

            if (begin == String::npos || end == String::npos)
                break;

            String objectInfo = message.substr(begin, end - begin + 1);
            // move handle to the end and add name if not indicated
            {
                uint64 handleStart = objectInfo.find(handleLabel);
                uint64 handleEnd = objectInfo.find(',', handleStart);
                String handleStr = objectInfo.substr(handleStart, handleEnd - handleStart);
                handleStr.insert(handleStr.begin(), ',');
                objectInfo.erase(handleStart, handleEnd - handleStart + 1);
                uint64 insertPos = objectInfo.find(';');
                objectInfo.insert(insertPos, handleStr);

                if (objectInfo.find(nameLabel) == String::npos)
                {
                    std::string label = " name = XXX,";
                    objectInfo.insert(1, label);
                }
            }

            objects.push_back(objectInfo);
            pos = message.find(objectLabel, end);
        }

        // Remove message part from object descriptions to message id inclusive
        uint64 deleteStart = message.find_first_of(']') + 1; 
        uint64 deleteEnd = message.find_last_of('|') + 1;
        message.erase(deleteStart, deleteEnd - deleteStart);

        // Add objects to end of the message in formmatted way
        message += "\n      Objects:";
        uint32 i = 0;
        for (const auto& objectInfo : objects)
            message += std::format("\n            {}{}", i++, objectInfo);
        message += "\n\n";

        switch (flags)
        {
        case VK_DEBUG_REPORT_INFORMATION_BIT_EXT:
            ATN_CORE_INFO_TAG("Vulkan", message); break;

        case VK_DEBUG_REPORT_WARNING_BIT_EXT:
            ATN_CORE_WARN_TAG("Vulkan", message); break;

        case VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT:
            ATN_CORE_WARN_TAG("Vulkan", message); break;

        case VK_DEBUG_REPORT_ERROR_BIT_EXT:
            ATN_CORE_ERROR_TAG("Vulkan", message);
            ATN_CORE_ASSERT(false);
            break;
        }

        return VK_FALSE;
    }

    inline VkShaderStageFlagBits GetShaderStage(ShaderStage stage)
	{
		switch (stage)
		{
		case ShaderStage::VERTEX_STAGE:   return VK_SHADER_STAGE_VERTEX_BIT;
		case ShaderStage::FRAGMENT_STAGE: return VK_SHADER_STAGE_FRAGMENT_BIT;
		case ShaderStage::GEOMETRY_STAGE: return VK_SHADER_STAGE_GEOMETRY_BIT;
		case ShaderStage::COMPUTE_STAGE:  return VK_SHADER_STAGE_COMPUTE_BIT;
		}

		ATN_CORE_ASSERT(false);
		return (VkShaderStageFlagBits)0;
	}

    inline VkFormat GetFormat(ImageFormat format)
    {
        switch (format)
        {
        case ImageFormat::RGB8:            return VK_FORMAT_R8G8B8_UNORM;
        case ImageFormat::RGB8_SRGB:       return VK_FORMAT_R8G8B8_SRGB;
        case ImageFormat::RGBA8:           return VK_FORMAT_R8G8B8A8_UNORM;
        case ImageFormat::RGBA8_SRGB:      return VK_FORMAT_R8G8B8A8_SRGB;

        case ImageFormat::RG16F:           return VK_FORMAT_R16G16_SFLOAT;
        case ImageFormat::R11G11B10F:      return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
        case ImageFormat::RGB16F:          return VK_FORMAT_R16G16B16_SFLOAT;
        case ImageFormat::RGB32F:          return VK_FORMAT_R32G32B32_SFLOAT;
        case ImageFormat::RGBA16F:         return VK_FORMAT_R16G16B16A16_SFLOAT;
        case ImageFormat::RGBA32F:         return VK_FORMAT_R32G32B32A32_SFLOAT;

        case ImageFormat::DEPTH16:         return VK_FORMAT_D16_UNORM;
        case ImageFormat::DEPTH24STENCIL8: return VK_FORMAT_D24_UNORM_S8_UINT;
        case ImageFormat::DEPTH32F:        return VK_FORMAT_D32_SFLOAT;
        }

        ATN_CORE_ASSERT(false);
        return (VkFormat)0;
    }

    inline VkFormat GetFormat(ShaderDataType type, bool isNormalized)
    {
        switch (type)
        {
        case ShaderDataType::Float:  return isNormalized ? VK_FORMAT_R16_UNORM : VK_FORMAT_R32_SFLOAT;
        case ShaderDataType::Float2: return isNormalized ? VK_FORMAT_R16G16_UNORM : VK_FORMAT_R32G32_SFLOAT;
        case ShaderDataType::Float3: return isNormalized ? VK_FORMAT_R16G16B16_UNORM : VK_FORMAT_R32G32B32_SFLOAT;
        case ShaderDataType::Float4: return isNormalized ? VK_FORMAT_R16G16B16A16_UNORM : VK_FORMAT_R32G32B32A32_SFLOAT;

        case ShaderDataType::Int:  return VK_FORMAT_R32_SINT;
        case ShaderDataType::Int2: return VK_FORMAT_R32G32_SINT;
        case ShaderDataType::Int3: return VK_FORMAT_R32G32B32_SINT;
        case ShaderDataType::Int4: return VK_FORMAT_R32G32B32A32_SINT;

        case ShaderDataType::UInt:  return VK_FORMAT_R32_UINT;
        }

        ATN_CORE_ASSERT(false);
        return (VkFormat)0;
    }

    inline VkImageAspectFlagBits GetImageAspectMask(ImageFormat format)
    {
        uint32 depthBit = Image::IsDepthFormat(format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_NONE;
        uint32 stencilBit = Image::IsStencilFormat(format) ? VK_IMAGE_ASPECT_STENCIL_BIT : VK_IMAGE_ASPECT_NONE;

        if (!depthBit && !stencilBit)
            return VK_IMAGE_ASPECT_COLOR_BIT;

        return VkImageAspectFlagBits(depthBit | stencilBit);
    }

    inline VkFilter GetFilter(TextureFilter filter)
    {
        switch (filter)
        {
        case TextureFilter::NEAREST:return VK_FILTER_NEAREST;
        case TextureFilter::LINEAR: return VK_FILTER_LINEAR;
        }

        ATN_CORE_ASSERT(false);
        return (VkFilter)0;
    }

    inline VkSamplerMipmapMode GetMipMapMode(TextureFilter filter)
    {
        switch (filter)
        {
        case TextureFilter::NEAREST:return VK_SAMPLER_MIPMAP_MODE_NEAREST;
        case TextureFilter::LINEAR: return VK_SAMPLER_MIPMAP_MODE_LINEAR;
        }

        ATN_CORE_ASSERT(false);
        return (VkSamplerMipmapMode)0;
    }

    inline VkSamplerAddressMode GetWrap(TextureWrap wrap)
    {
        switch (wrap)
        {
        case TextureWrap::REPEAT: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case TextureWrap::CLAMP_TO_EDGE: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case TextureWrap::CLAMP_TO_BORDER: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        case TextureWrap::MIRRORED_REPEAT: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        case TextureWrap::MIRRORED_CLAMP_TO_EDGE: return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
        }

        ATN_CORE_ASSERT(false);
        return (VkSamplerAddressMode)0;
    }

    static VkShaderStageFlags GetShaderStageFlags(ShaderStage stageFlags)
    {
        uint64 flags = 0;

        if (stageFlags & ShaderStage::VERTEX_STAGE)
            flags |= VK_SHADER_STAGE_VERTEX_BIT;

        if (stageFlags & ShaderStage::FRAGMENT_STAGE)
            flags |= VK_SHADER_STAGE_FRAGMENT_BIT;

        if (stageFlags & ShaderStage::GEOMETRY_STAGE)
            flags |= VK_SHADER_STAGE_GEOMETRY_BIT;

        if (stageFlags & ShaderStage::COMPUTE_STAGE)
            flags |= VK_SHADER_STAGE_COMPUTE_BIT;

        return flags;
    }

    inline VkCommandBuffer BeginSingleTimeCommands()
    {
        VkCommandBufferAllocateInfo cmdBufAllocInfo = {};
        cmdBufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdBufAllocInfo.commandPool = VulkanContext::GetCommandPool();
        cmdBufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdBufAllocInfo.commandBufferCount = 1;

        VkCommandBuffer vkCommandBuffer;
        VK_CHECK(vkAllocateCommandBuffers(VulkanContext::GetLogicalDevice(), &cmdBufAllocInfo, &vkCommandBuffer));

        VkCommandBufferBeginInfo cmdBufBeginInfo = {};
        cmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdBufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VK_CHECK(vkBeginCommandBuffer(vkCommandBuffer, &cmdBufBeginInfo));

        return vkCommandBuffer;
    }

    inline void EndSingleTimeCommands(VkCommandBuffer vkCommandBuffer)
    {
        vkEndCommandBuffer(vkCommandBuffer);

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &vkCommandBuffer;

        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = 0;

        VkFence fence;
        VK_CHECK(vkCreateFence(VulkanContext::GetLogicalDevice(), &fenceInfo, nullptr, &fence));

        VK_CHECK(vkQueueSubmit(VulkanContext::GetDevice()->GetQueue(), 1, &submitInfo, fence));

        VK_CHECK(vkWaitForFences(VulkanContext::GetLogicalDevice(), 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));

        vkDestroyFence(VulkanContext::GetLogicalDevice(), fence, nullptr);
        vkFreeCommandBuffers(VulkanContext::GetLogicalDevice(), VulkanContext::GetCommandPool(), 1, &vkCommandBuffer);
    }
}
