#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#define VMA_IMPLEMENTATION

// Disable vma asserts. Vma checks if all allocations freed on shutdown, 
// when application is closed by closing console, some resources are not freed(unknown which resources)
// so vma crushes whole application. So this problem causing console.
#define VMA_ASSERT

#include <vma/vk_mem_alloc.h>

#include "VulkanAllocator.h"
#include "Athena/Platform/Vulkan/VulkanUtils.h"


namespace Athena
{
	namespace Utils
	{
		static float BytesToKb(VkDeviceSize size)
		{
			return (float)size / 1024.f;
		}
	}

	void* VulkanBuffer::MapMemory()
	{
		void* mappedData;
		VK_CHECK(vmaMapMemory(VulkanContext::GetAllocator()->GetInternalAllocator(), m_Allocation, &mappedData));
		return mappedData;
	}

	void VulkanBuffer::UnmapMemory()
	{
		vmaUnmapMemory(VulkanContext::GetAllocator()->GetInternalAllocator(), m_Allocation);
	}

	VulkanAllocator::VulkanAllocator(uint32 vulkanVersion)
	{
		VmaVulkanFunctions vulkanFunctions = {};
		vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
		vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

		VmaAllocatorCreateInfo allocatorCreateInfo = {};
		allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
		allocatorCreateInfo.vulkanApiVersion = vulkanVersion;
		allocatorCreateInfo.physicalDevice = VulkanContext::GetPhysicalDevice();
		allocatorCreateInfo.device = VulkanContext::GetLogicalDevice();
		allocatorCreateInfo.instance = VulkanContext::GetInstance();
		allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

		VK_CHECK(vmaCreateAllocator(&allocatorCreateInfo, &m_Allocator));

		vkGetPhysicalDeviceMemoryProperties(VulkanContext::GetPhysicalDevice(), &m_MemoryProps);
	}

	VulkanAllocator::~VulkanAllocator()
	{
		vmaDestroyAllocator(m_Allocator);
	}

	VulkanBuffer VulkanAllocator::AllocateBuffer(const VkBufferCreateInfo& bufferInfo, VmaMemoryUsage usage, VmaAllocationCreateFlagBits flags)
	{
		VkBuffer buffer;
		VmaAllocation allocation;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = usage;
		allocInfo.flags = flags;

		VK_CHECK(vmaCreateBuffer(m_Allocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr));
		ATN_CORE_TRACE_TAG("Vulkan", "Allocating buffer of {} Kb", Utils::BytesToKb(allocation->GetSize()));

		return VulkanBuffer(buffer, allocation);
	}

	VulkanImage VulkanAllocator::AllocateImage(const VkImageCreateInfo& imageInfo, VmaMemoryUsage usage, VmaAllocationCreateFlagBits flags)
	{
		VkImage image;
		VmaAllocation allocation;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = usage;
		allocInfo.flags = flags;

		VK_CHECK(vmaCreateImage(m_Allocator, &imageInfo, &allocInfo, &image, &allocation, nullptr));
		ATN_CORE_TRACE_TAG("Vulkan", "Allocating image of {} Kb", Utils::BytesToKb(allocation->GetSize()));

		return VulkanImage(image, allocation);
	}

	void VulkanAllocator::DestroyBuffer(VulkanBuffer buffer)
	{
		ATN_CORE_TRACE_TAG("Vulkan", "Destroying buffer of {} Kb", Utils::BytesToKb(buffer.GetAllocation()->GetSize()));
		vmaDestroyBuffer(m_Allocator, buffer.GetBuffer(), buffer.GetAllocation());
	}

	void VulkanAllocator::DestroyImage(VulkanImage image)
	{
		ATN_CORE_TRACE_TAG("Vulkan", "Destroying image of {} Kb", Utils::BytesToKb(image.GetAllocation()->GetSize()));
		vmaDestroyImage(m_Allocator, image.GetImage(), image.GetAllocation());
	}

	void VulkanAllocator::OnUpdate()
	{
		vmaSetCurrentFrameIndex(m_Allocator, Renderer::GetCurrentFrameIndex());
	}

	uint64 VulkanAllocator::GetMemoryUsage()
	{
		VmaBudget heaps[VK_MAX_MEMORY_HEAPS] = { 0 };
		vmaGetHeapBudgets(m_Allocator, heaps);

		uint64 result = 0;
		for (uint32 i = 0; i < m_MemoryProps.memoryHeapCount; i++)
		{
			if(m_MemoryProps.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
				result += heaps[i].usage;
		}

		return result;
	}
}
