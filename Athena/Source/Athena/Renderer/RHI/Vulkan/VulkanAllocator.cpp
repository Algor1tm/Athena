#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#define VMA_IMPLEMENTATION

#include <vma/vk_mem_alloc.h>

#include "VulkanAllocator.h"
#include "Athena/Math/Common.h"
#include "Athena/Utils/StringUtils.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanRHI.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanUtils.h"


namespace Athena
{
	namespace Vulkan
	{
		inline VkFilter TranslateFilter(TextureFilter filter)
		{
			switch (filter)
			{
			case TextureFilter::NEAREST:return VK_FILTER_NEAREST;
			case TextureFilter::LINEAR: return VK_FILTER_LINEAR;
			case TextureFilter::TRILINEAR: return VK_FILTER_LINEAR;
			}

			checkf(false);
			return (VkFilter)0;
		}

		inline VkSamplerMipmapMode TranslateMipMapMode(TextureFilter filter)
		{
			switch (filter)
			{
			case TextureFilter::NEAREST:return VK_SAMPLER_MIPMAP_MODE_NEAREST;
			case TextureFilter::LINEAR: return VK_SAMPLER_MIPMAP_MODE_NEAREST;
			case TextureFilter::TRILINEAR: return VK_SAMPLER_MIPMAP_MODE_LINEAR;
			}

			checkf(false);
			return (VkSamplerMipmapMode)0;
		}

		inline VkSamplerAddressMode TranslateWrap(TextureWrap wrap)
		{
			switch (wrap)
			{
			case TextureWrap::REPEAT:                 return VK_SAMPLER_ADDRESS_MODE_REPEAT;
			case TextureWrap::CLAMP_TO_EDGE:          return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			case TextureWrap::CLAMP_TO_BORDER:        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			case TextureWrap::MIRRORED_REPEAT:        return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			case TextureWrap::MIRRORED_CLAMP_TO_EDGE: return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
			}

			checkf(false);
			return (VkSamplerAddressMode)0;
		}

		inline VkCompareOp TranslateCompareOp(TextureCompareOperator compare)
		{
			switch (compare)
			{
			case TextureCompareOperator::NONE:             return VK_COMPARE_OP_NEVER;
			case TextureCompareOperator::LESS_OR_EQUAL:    return VK_COMPARE_OP_LESS_OR_EQUAL;
			case TextureCompareOperator::GREATER_OR_EQUAL: return VK_COMPARE_OP_GREATER_OR_EQUAL;
			}

			checkf(false);
			return (VkCompareOp)0;
		}
	}

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

	VkSampler VulkanAllocator::CreateSampler(const TextureSamplerCreateInfo& info)
	{
		if (m_SamplersMap.contains(info))
		{
			auto& samplerInfo = m_SamplersMap.at(info);
			samplerInfo.RefCount++;
			return samplerInfo.Sampler;
		}

		VkSampler sampler;

		float maxAnisotropy = Math::Min(info.AnisotropyLevel, RHI::Get()->GetGPUCapabilities().MaxSamplerAnisotropy);

		VkSamplerCreateInfo vksamplerInfo = {};
		vksamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		vksamplerInfo.magFilter = Vulkan::TranslateFilter(info.Filter);
		vksamplerInfo.minFilter = Vulkan::TranslateFilter(info.Filter);
		vksamplerInfo.mipmapMode = Vulkan::TranslateMipMapMode(info.Filter);
		vksamplerInfo.addressModeU = Vulkan::TranslateWrap(info.Wrap);
		vksamplerInfo.addressModeV = Vulkan::TranslateWrap(info.Wrap);
		vksamplerInfo.addressModeW = Vulkan::TranslateWrap(info.Wrap);
		vksamplerInfo.anisotropyEnable = maxAnisotropy > 1.f;
		vksamplerInfo.maxAnisotropy = maxAnisotropy;
		vksamplerInfo.compareEnable = info.Compare == TextureCompareOperator::NONE ? false : true;
		vksamplerInfo.compareOp = Vulkan::TranslateCompareOp(info.Compare);
		vksamplerInfo.minLod = 0;
		vksamplerInfo.maxLod = VK_LOD_CLAMP_NONE;
		vksamplerInfo.mipLodBias = 0.f;
		vksamplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		vksamplerInfo.unnormalizedCoordinates = false;

		VK_CHECK(vkCreateSampler(Vulkan::GetLogicalDevice(), &vksamplerInfo, nullptr, &sampler));

		m_SamplersMap[info] = { sampler, 1 };
		return sampler;
	}

	void VulkanAllocator::DestroySampler(const TextureSamplerCreateInfo& info, VkSampler sampler)
	{
		checkf(m_SamplersMap.contains(info));

		auto& samplerInfo = m_SamplersMap.at(info);
		checkf(samplerInfo.Sampler == sampler);

		samplerInfo.RefCount--;

		if (samplerInfo.RefCount == 0)
		{
			vkDestroySampler(Vulkan::GetLogicalDevice(), samplerInfo.Sampler, nullptr);
			m_SamplersMap.erase(info);
		}
	}

	void VulkanAllocator::NewFrame()
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
