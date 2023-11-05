#pragma once

#include "Athena/Core/Core.h"

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>


namespace Athena
{
	class VulkanBuffer
	{
	public:
		VulkanBuffer() = default;

		VulkanBuffer(VkBuffer buffer, VmaAllocation alloc)
			: m_Buffer(buffer), m_Allocation(alloc)
		{}

		VkBuffer GetBuffer() { return m_Buffer; }
		VmaAllocation GetAllocation() { return m_Allocation; }

		void* MapMemory();
		void UnmapMemory();

	private:
		VkBuffer m_Buffer;
		VmaAllocation m_Allocation;
	};
	
	class VulkanImage
	{
	public:
		VulkanImage() = default;

		VulkanImage(VkImage image, VmaAllocation alloc)
			: m_Image(image), m_Allocation(alloc)
		{}

		VkImage GetImage() { return m_Image; }
		VmaAllocation GetAllocation() { return m_Allocation; }

	private:
		VkImage m_Image;
		VmaAllocation m_Allocation;
	};


	class VulkanAllocator : public RefCounted
	{
	public:
		VulkanAllocator(uint32 vulkanVersion);
		~VulkanAllocator();

		VulkanBuffer AllocateBuffer(const VkBufferCreateInfo& bufferInfo, VmaMemoryUsage usage = VMA_MEMORY_USAGE_AUTO, VmaAllocationCreateFlagBits flags = (VmaAllocationCreateFlagBits)0);
		VulkanImage AllocateImage(const VkImageCreateInfo& imageInfo, VmaMemoryUsage usage = VMA_MEMORY_USAGE_AUTO, VmaAllocationCreateFlagBits flags = (VmaAllocationCreateFlagBits)0);

		void DestroyBuffer(VulkanBuffer buffer);
		void DestroyImage(VulkanImage image);

		VmaAllocator GetInternalAllocator() { return m_Allocator; }

	private:
		VmaAllocator m_Allocator;
	};
}
