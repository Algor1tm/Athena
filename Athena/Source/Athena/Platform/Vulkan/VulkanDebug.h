#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Log.h"

#include <vulkan/vulkan.h>


namespace Athena
{
#ifndef ATN_DEBUG

    #define VK_CHECK(expr)

#else
    // Copied from glfw internal
    inline const char* GetVulkanResultString(VkResult result)
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

	inline void CheckVulkanResult(VkResult error)
	{
		if (error == 0)
			return;

        const char* errorString = GetVulkanResultString(error);

		if (error > 0)
			ATN_CORE_ERROR_TAG("Vulkan", "VkResult = {}, Error: {}", (int)error, errorString);

		if (error < 0)
			ATN_CORE_FATAL_TAG("Vulkan", "VkResult = {}, Fatal Error: {}", (int)error, errorString);

		ATN_CORE_ASSERT(false);
	}


    #define VK_CHECK(expr) CheckVulkanResult(expr)


    inline const char* GetDebugObjectTypeString(VkDebugReportObjectTypeEXT objectType)
    {
        switch (objectType)
        {
        case VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT: 
            return "INSTANCE";
        case VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT:
            return "PHYSICAL_DEVICE";
        case VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT:
            return "DEVICE";
        case VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT:
            return "QUEUE";
        case VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT:
            return "SEMAPHORE";
        case VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT:
            return "COMMAND_BUFFER";
        case VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT:
            return "FENCE";
        case VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT:
            return "DEVICE_MEMORY";
        case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT:
            return "BUFFER";
        case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT:
            return "IMAGE";
        case VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT:
            return "EVENT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT:
            return "QUERY_POOL";
        case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT:
            return "BUFFER_VIEW";
        case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT:
            return "IMAGE_VIEW";
        case VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT:
            return "SHADER_MODULE";
        case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT:
            return "PIPELINE_CACHE";
        case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT:
            return "PIPELINE_LAYOUT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT:
            return "RENDER_PASS";
        case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT:
            return "PIPELINE";
        case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT:
            return "DESCRIPTOR_SET_LAYOUT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT:
            return "SAMPLER";
        case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT:
            return "DESCRIPTOR_POOL";
        case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT:
            return "DESCRIPTOR_SET";
        case VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT:
            return "FRAMEBUFFER";
        case VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT:
            return "COMMAND_POOL";
        case VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT:
            return "SURFACE_KHR";
        case VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT:
            return "SWAPCHAIN_KHR";
        case VK_DEBUG_REPORT_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT_EXT:
            return "DEBUG_REPORT_CALLBACK_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_DISPLAY_KHR_EXT:
            return "DISPLAY_KHR";
        case VK_DEBUG_REPORT_OBJECT_TYPE_DISPLAY_MODE_KHR_EXT:
            return "DISPLAY_MODE_KHR";
        case VK_DEBUG_REPORT_OBJECT_TYPE_VALIDATION_CACHE_EXT_EXT:
            return "VALIDATION_CACHE_EXT";
        default:
            return "UNKNOWN_OBJECT_TYPE";
        }
    }

	inline VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType,
		uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
	{
        const char* objectTypeString = GetDebugObjectTypeString(objectType);

        switch (flags)
        {
        case VK_DEBUG_REPORT_INFORMATION_BIT_EXT: 
            ATN_CORE_INFO_TAG("Vulkan", "Debug report from ObjectType({}): '{}' \nMessage: {}\n", (int)objectType, objectTypeString, pMessage); break;

        case VK_DEBUG_REPORT_WARNING_BIT_EXT: 
            ATN_CORE_WARN_TAG("Vulkan", "Debug report from ObjectType({}): '{}'  \nMessage: {}\n", (int)objectType, objectTypeString, pMessage); break;

        case VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT: 
            ATN_CORE_WARN_TAG("Vulkan", "'PERFORMANCE WARNING' Debug report from ObjectType({}): '{}'  \nMessage: {}\n", (int)objectType, objectTypeString, pMessage); break;

        case VK_DEBUG_REPORT_ERROR_BIT_EXT: 
            ATN_CORE_ERROR_TAG("Vulkan", "Debug report from ObjectType({}): '{}'  \nMessage: {}\n", (int)objectType, objectTypeString, pMessage); break;
        }

		return VK_FALSE;
	}
#endif
}
