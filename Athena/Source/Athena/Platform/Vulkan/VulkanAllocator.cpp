#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#define VMA_IMPLEMENTATION
#include "vma/vk_mem_alloc.h"

#include "VulkanAllocator.h"
#include "Athena/Platform/Vulkan/VulkanUtils.h"


namespace Athena
{
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
		allocatorCreateInfo.vulkanApiVersion = vulkanVersion;
		allocatorCreateInfo.physicalDevice = VulkanContext::GetPhysicalDevice();
		allocatorCreateInfo.device = VulkanContext::GetLogicalDevice();
		allocatorCreateInfo.instance = VulkanContext::GetInstance();
		allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

		VK_CHECK(vmaCreateAllocator(&allocatorCreateInfo, &m_Allocator));
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
		return VulkanImage(image, allocation);
	}

	void VulkanAllocator::DestroyBuffer(VulkanBuffer buffer)
	{
		vmaDestroyBuffer(m_Allocator, buffer.GetBuffer(), buffer.GetAllocation());
	}

	void VulkanAllocator::DestroyImage(VulkanImage image)
	{
		vmaDestroyImage(m_Allocator, image.GetImage(), image.GetAllocation());
	}
}
