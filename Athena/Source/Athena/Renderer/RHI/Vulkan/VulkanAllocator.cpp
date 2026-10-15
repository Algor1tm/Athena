#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#define VMA_IMPLEMENTATION

#include <vma/vk_mem_alloc.h>

#include "VulkanAllocator.h"
#include "Athena/Utils/StringUtils.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanRHI.h"


namespace Athena
{
	void* VulkanBufferAllocation::MapMemory()
	{
		void* mappedData;
		VK_CHECK(vmaMapMemory(VulkanRHI::Get()->GetAllocator()->GetInternalAllocator(), m_Allocation, &mappedData));
		return mappedData;
	}

	void VulkanBufferAllocation::UnmapMemory()
	{
		vmaUnmapMemory(VulkanRHI::Get()->GetAllocator()->GetInternalAllocator(), m_Allocation);
	}

	VulkanAllocator::VulkanAllocator()
	{
		VmaVulkanFunctions vulkanFunctions = {};
		vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
		vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

		const DeviceExtensionsInfo& deviceExtensions = VulkanRHI::Get()->GetDevice()->GetExtensionsInfo();
		const InstanceExtensionsInfo& instanceExtensions = VulkanRHI::Get()->GetExtensionsInfo();

		m_GetMemoryUsageSupported = deviceExtensions.MemoryBudgetEXTSupported && instanceExtensions.GetPhysicalDeviceProperties2EXTSupported;

		VmaAllocatorCreateFlags flags = VmaAllocatorCreateFlags(0);

		if (m_GetMemoryUsageSupported)
		{
			flags = flags | VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
		}
		else
		{
			ATN_LOG_WARN(Vulkan, "Vulkan device does not support GetMemoryUsage (VRAM)");
		}

		VmaAllocatorCreateInfo allocatorCreateInfo = {};
		allocatorCreateInfo.flags = flags;
		allocatorCreateInfo.vulkanApiVersion = VulkanRHI::Get()->GetVulkanVersion();
		allocatorCreateInfo.physicalDevice = VulkanRHI::Get()->GetDevice()->GetPhysicalDevice();
		allocatorCreateInfo.device = VulkanRHI::Get()->GetDevice()->GetLogicalDevice();
		allocatorCreateInfo.instance = VulkanRHI::Get()->GetVulkanInstance();
		allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

		VK_CHECK(vmaCreateAllocator(&allocatorCreateInfo, &m_Allocator));

		vkGetPhysicalDeviceMemoryProperties(VulkanRHI::Get()->GetDevice()->GetPhysicalDevice(), &m_MemoryProps);
	}

	VulkanAllocator::~VulkanAllocator()
	{
		vmaDestroyAllocator(m_Allocator);
	}

	VulkanBufferAllocation VulkanAllocator::AllocateBuffer(const VkBufferCreateInfo& bufferInfo, VmaMemoryUsage usage, VmaAllocationCreateFlags flags, const String& name)
	{
		VkBuffer buffer;
		VmaAllocation allocation;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = usage;
		allocInfo.flags = flags;

		VK_CHECK(vmaCreateBuffer(m_Allocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr));

		VkDeviceSize size = allocation->GetSize();

#if VULKAN_ENABLE_MEMORY_DEBUG_INFO
		if (!name.empty())
			ATN_LOG_TRACE(Renderer, "Allocating buffer '{}' {}", name, Utils::MemoryBytesToString(size));
#endif

		return VulkanBufferAllocation(buffer, allocation);
	}

	VulkanImageAllocation VulkanAllocator::AllocateImage(const VkImageCreateInfo& imageInfo, VmaMemoryUsage usage, VmaAllocationCreateFlags flags, const String& name)
	{
		VkImage image;
		VmaAllocation allocation;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = usage;
		allocInfo.flags = flags;

		VK_CHECK(vmaCreateImage(m_Allocator, &imageInfo, &allocInfo, &image, &allocation, nullptr));

#if VULKAN_ENABLE_MEMORY_DEBUG_INFO
		VkDeviceSize size = allocation->GetSize();
		ATN_LOG_TRACE(Renderer, "Allocating image '{}' {}", name, Utils::MemoryBytesToString(size));
#endif

		return VulkanImageAllocation(image, allocation);
	}

	void VulkanAllocator::DestroyBuffer(VulkanBufferAllocation buffer, const String& name)
	{
		VkDeviceSize size = buffer.GetAllocation()->GetSize();

#if VULKAN_ENABLE_MEMORY_DEBUG_INFO
		if (!name.empty())
			ATN_LOG_TRACE(Renderer, "Destroying buffer '{}' {}", name, Utils::MemoryBytesToString(size));
#endif

		vmaDestroyBuffer(m_Allocator, buffer.GetBuffer(), buffer.GetAllocation());
	}

	void VulkanAllocator::DestroyImage(VulkanImageAllocation image, const String& name)
	{
#if VULKAN_ENABLE_MEMORY_DEBUG_INFO
		VkDeviceSize size = image.GetAllocation()->GetSize();
		ATN_LOG_TRACE(Renderer, "Destroying image '{}' {}", name, Utils::MemoryBytesToString(size));
#endif

		vmaDestroyImage(m_Allocator, image.GetImage(), image.GetAllocation());
	}

	void VulkanAllocator::OnUpdate()
	{
		vmaSetCurrentFrameIndex(m_Allocator, VulkanRHI::Get()->GetFrameResourcesIndex());
	}

	uint64 VulkanAllocator::GetMemoryUsage()
	{
		if (!m_GetMemoryUsageSupported)
			return 0;

		VmaBudget heaps[VK_MAX_MEMORY_HEAPS] = { 0 };
		vmaGetHeapBudgets(m_Allocator, heaps);

		uint64 result = 0;
		for (uint32 i = 0; i < m_MemoryProps.memoryHeapCount; i++)
		{
			if (m_MemoryProps.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
				result += heaps[i].usage;
		}

		return result;
	}
}