#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Color.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanConfiguration.h"

#include <vulkan/vulkan.h>


namespace Athena::Vulkan
{
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

    inline const char* VkObjectTypeToString(VkObjectType objectType)
    {
        switch (objectType)
        {
        case VK_OBJECT_TYPE_UNKNOWN:               return "VK_OBJECT_TYPE_UNKNOWN";
        case VK_OBJECT_TYPE_INSTANCE:              return "VK_OBJECT_TYPE_INSTANCE";
        case VK_OBJECT_TYPE_PHYSICAL_DEVICE:       return "VK_OBJECT_TYPE_PHYSICAL_DEVICE";
        case VK_OBJECT_TYPE_DEVICE:                return "VK_OBJECT_TYPE_DEVICE";
        case VK_OBJECT_TYPE_QUEUE:                 return "VK_OBJECT_TYPE_QUEUE";
        case VK_OBJECT_TYPE_SEMAPHORE:             return "VK_OBJECT_TYPE_SEMAPHORE";
        case VK_OBJECT_TYPE_COMMAND_BUFFER:        return "VK_OBJECT_TYPE_COMMAND_BUFFER";
        case VK_OBJECT_TYPE_FENCE:                 return "VK_OBJECT_TYPE_FENCE";
        case VK_OBJECT_TYPE_DEVICE_MEMORY:         return "VK_OBJECT_TYPE_DEVICE_MEMORY";
        case VK_OBJECT_TYPE_BUFFER:                return "VK_OBJECT_TYPE_BUFFER";
        case VK_OBJECT_TYPE_IMAGE:                 return "VK_OBJECT_TYPE_IMAGE";
        case VK_OBJECT_TYPE_IMAGE_VIEW:            return "VK_OBJECT_TYPE_IMAGE_VIEW";
        case VK_OBJECT_TYPE_SHADER_MODULE:         return "VK_OBJECT_TYPE_SHADER_MODULE";
        case VK_OBJECT_TYPE_PIPELINE_LAYOUT:       return "VK_OBJECT_TYPE_PIPELINE_LAYOUT";
        case VK_OBJECT_TYPE_RENDER_PASS:           return "VK_OBJECT_TYPE_RENDER_PASS";
        case VK_OBJECT_TYPE_PIPELINE:              return "VK_OBJECT_TYPE_PIPELINE";
        case VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT: return "VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT";
        case VK_OBJECT_TYPE_SAMPLER:               return "VK_OBJECT_TYPE_SAMPLER";
        case VK_OBJECT_TYPE_DESCRIPTOR_POOL:       return "VK_OBJECT_TYPE_DESCRIPTOR_POOL";
        case VK_OBJECT_TYPE_DESCRIPTOR_SET:        return "VK_OBJECT_TYPE_DESCRIPTOR_SET";
        case VK_OBJECT_TYPE_FRAMEBUFFER:           return "VK_OBJECT_TYPE_FRAMEBUFFER";
        case VK_OBJECT_TYPE_COMMAND_POOL:          return "VK_OBJECT_TYPE_COMMAND_POOL";
        case VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION: return "VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION";
        case VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE: return "VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE";
        case VK_OBJECT_TYPE_SURFACE_KHR:           return "VK_OBJECT_TYPE_SURFACE_KHR";
        case VK_OBJECT_TYPE_SWAPCHAIN_KHR:         return "VK_OBJECT_TYPE_SWAPCHAIN_KHR";
        case VK_OBJECT_TYPE_DISPLAY_KHR:           return "VK_OBJECT_TYPE_DISPLAY_KHR";
        case VK_OBJECT_TYPE_DISPLAY_MODE_KHR:      return "VK_OBJECT_TYPE_DISPLAY_MODE_KHR";
        case VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT: return "VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT";
        case VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT: return "VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT";
        default:                                   return "VK_OBJECT_TYPE_UNKNOWN_OR_EXTENSION";
        }
    }

    inline bool CheckResult(VkResult error)
    {
        if (error == 0)
            return true;

        const char* errorString = GetResultString(error);

        if (error == VK_ERROR_DEVICE_LOST)
        {
            ATN_LOG_ERROR(Vulkan, "GPU CRASHED!");
        }

        if (error > 0)
            ATN_LOG_ERROR(Vulkan, "VkResult = {}, Error: {}", (int)error, errorString);

        if (error < 0)
            ATN_LOG_FATAL(Vulkan, "VkResult = {}, Fatal Error: {}", (int)error, errorString);

        return false;
    }

    #if ATN_ENABLE_ENSURES
        #define VK_CHECK(expr) ensuref(::Athena::Vulkan::CheckResult(expr))
    #else
        #define VK_CHECK(expr) expr
    #endif

#if VULKAN_ENABLE_DEBUG_INFO
    inline VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, 
        VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
    {
        switch (messageSeverity)
        {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            ATN_LOG_INFO(Vulkan, pCallbackData->pMessage);
            break;

        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            ATN_LOG_WARN(Vulkan, pCallbackData->pMessage);
            break;

        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            ATN_LOG_WARN(Vulkan, pCallbackData->pMessage);
            break;

        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            ATN_LOG_ERROR(Vulkan, pCallbackData->pMessage);
            break;
        }

        if (pCallbackData->objectCount > 0) 
        {
            ATN_LOG_TRACE(Vulkan, "  Associated Objects:");
            for (uint32_t i = 0; i < pCallbackData->objectCount; ++i) 
            {
                auto& obj = pCallbackData->pObjects[i];
                ATN_LOG_TRACE(Vulkan, "    [{}] Type: {}, Handle: {}, Name: {}",
                    i,
                    VkObjectTypeToString(obj.objectType),
                    (void*)obj.objectHandle,
                    obj.pObjectName ? obj.pObjectName : "UNKNOWN"
                );
            }
        }

        if (pCallbackData->cmdBufLabelCount > 0)
        {
            ATN_LOG_TRACE(Vulkan, "  Active Command Buffer Labels:");
            for (uint32_t i = 0; i < pCallbackData->cmdBufLabelCount; ++i)
            {
                ATN_LOG_TRACE(Vulkan, "    [{}] {}", i, pCallbackData->pCmdBufLabels[i].pLabelName);
            }
        }

        //if (pCallbackData->queueLabelCount > 0) 
        //{
        //    ATN_LOG_INFO(Vulkan, "  Active Queue Labels:\n");
        //    for (uint32_t i = 0; i < pCallbackData->queueLabelCount; ++i) 
        //    {
        //        ATN_LOG_INFO(Vulkan, "    [%u] %s\n", i, pCallbackData->pQueueLabels[i].pLabelName);
        //    }
        //}

        ensuref(false);

        return VK_FALSE;
    }
#endif


#if VULKAN_ENABLE_DEBUG_INFO
    void SetDebugName(void* objectHandle, VkObjectType type, const String& name);
    void BeginDebugLabel(VkCommandBuffer cmdBuffer, const String& name, const IntegerColor& color);
    void EndDebugLabel(VkCommandBuffer cmdBuffer);
    void InsertDebugLabel(VkCommandBuffer cmdBuffer, const String& name, const IntegerColor& color);


    #define VK_SET_OBJECT_NAME(object, type, name) Vulkan::SetDebugName(object, type, name)
    #define VK_BEGIN_DEBUG_LABEL(cmdBuffer, name, color) Vulkan::BeginDebugLabel(cmdBuffer, name, color)
    #define VK_END_DEBUG_LABEL(cmdBuffer) Vulkan::EndDebugLabel(cmdBuffer)
    #define VK_INSERT_DEBUG_LABEL(cmdBuffer, name, color) Vulkan::InsertDebugLabel(cmdBuffer, name, color)
#else
    #define VK_SET_OBJECT_NAME(object, type, name) 
    #define VK_BEGIN_DEBUG_LABEL(cmdBuffer, name, color)
    #define VK_END_DEBUG_LABEL(cmdBuffer)
    #define VK_INSERT_DEBUG_LABEL(cmdBuffer, name, color)
#endif
}
