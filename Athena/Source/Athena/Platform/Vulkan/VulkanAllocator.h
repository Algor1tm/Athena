#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Texture.h"

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>


namespace std
{
	using namespace Athena;

	template<>
	struct hash<TextureSamplerCreateInfo>
	{
		size_t operator()(const TextureSamplerCreateInfo& value) const
		{
			string str = std::format("{}{}{}{}{}", (uint32)value.MinFilter, (uint32)value.MagFilter, 
				(uint32)value.MipMapFilter, (uint32)value.Wrap, (uint32)value.Compare);

			return hash<string>()(str);
		}
	};
}

namespace Athena
{
	class VulkanBufferAllocation
	{
	public:
		VulkanBufferAllocation() = default;

		VulkanBufferAllocation(VkBuffer buffer, VmaAllocation alloc)
			: m_Buffer(buffer), m_Allocation(alloc)
		{}

		VkBuffer GetBuffer() const { return m_Buffer; }
		VmaAllocation GetAllocation() const { return m_Allocation; }

		void* MapMemory();
		void UnmapMemory();

		operator bool() const { return !(m_Buffer == VK_NULL_HANDLE && m_Allocation == VK_NULL_HANDLE); }

	private:
		VkBuffer m_Buffer = VK_NULL_HANDLE;
		VmaAllocation m_Allocation = VK_NULL_HANDLE;
	};
	
	class VulkanImageAllocation
	{
	public:
		VulkanImageAllocation() = default;

		VulkanImageAllocation(VkImage image, VmaAllocation alloc)
			: m_Image(image), m_Allocation(alloc)
		{}

		VkImage GetImage() const { return m_Image; }
		VmaAllocation GetAllocation() const { return m_Allocation; }

		operator bool() const { return !(m_Image == VK_NULL_HANDLE && m_Allocation == VK_NULL_HANDLE); }

	private:
		VkImage m_Image = VK_NULL_HANDLE;
		VmaAllocation m_Allocation = VK_NULL_HANDLE;
	};

	struct VulkanSamplerAllocation
	{
		VkSampler Sampler = VK_NULL_HANDLE;
		uint64 RefCount = 0;
	};


	class VulkanAllocator : public RefCounted
	{
	public:
		VulkanAllocator(uint32 vulkanVersion);
		~VulkanAllocator();

		VulkanBufferAllocation AllocateBuffer(const VkBufferCreateInfo& bufferInfo, VmaMemoryUsage usage = VMA_MEMORY_USAGE_AUTO, VmaAllocationCreateFlagBits flags = (VmaAllocationCreateFlagBits)0, const String& name = "");
		VulkanImageAllocation AllocateImage(const VkImageCreateInfo& imageInfo, VmaMemoryUsage usage = VMA_MEMORY_USAGE_AUTO, VmaAllocationCreateFlagBits flags = (VmaAllocationCreateFlagBits)0, const String& name = "");

		void DestroyBuffer(VulkanBufferAllocation buffer, const String& name = "");
		void DestroyImage(VulkanImageAllocation image, const String& name = "");

		VmaAllocator GetInternalAllocator() { return m_Allocator; }

		VkSampler AllocateSampler(const TextureSamplerCreateInfo& info);
		void DestroySampler(const TextureSamplerCreateInfo& info, VkSampler sampler);

		void OnUpdate();

		// return in Kb
		uint64 GetMemoryUsage();

	private:
		VmaAllocator m_Allocator;
		VkPhysicalDeviceMemoryProperties m_MemoryProps;
		std::unordered_map<TextureSamplerCreateInfo, VulkanSamplerAllocation> m_SamplersMap;
	};
}
