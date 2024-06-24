#include "DescriptorSetManager.h"

#include "Athena/Platform/Vulkan/VulkanTexture2D.h"
#include "Athena/Platform/Vulkan/VulkanTextureCube.h"
#include "Athena/Platform/Vulkan/VulkanTextureView.h"
#include "Athena/Platform/Vulkan/VulkanUniformBuffer.h"
#include "Athena/Platform/Vulkan/VulkanStorageBuffer.h"
#include "Athena/Platform/Vulkan/VulkanShader.h"
#include "Athena/Platform/Vulkan/VulkanUtils.h"
#include "Athena/Renderer/TextureGenerator.h"


namespace Athena
{
	namespace Vulkan
	{
		static VkDescriptorType GetDescriptorType(ShaderResourceType type)
		{
			switch (type)
			{
			case ShaderResourceType::Texture2D:      return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			case ShaderResourceType::StorageTexture2D:	 return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			case ShaderResourceType::TextureCube:    return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			case ShaderResourceType::StorageTextureCube:  return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			case ShaderResourceType::UniformBuffer:	 return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			case ShaderResourceType::StorageBuffer:	 return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			}

			ATN_CORE_ASSERT(false);
			return (VkDescriptorType)0;
		}
	}

	namespace Utils
	{
		static std::string_view ResourceTypeToString(RenderResourceType type)
		{
			switch(type)
			{
			case RenderResourceType::Texture2D:        return "Texture2D";
			case RenderResourceType::TextureCube:      return "TextureCube";
			case RenderResourceType::TextureView2D:    return "TextureView2D";
			case RenderResourceType::TextureViewCube:  return "TextureViewCube";
			case RenderResourceType::UniformBuffer:	   return "UniformBuffer";
			case RenderResourceType::StorageBuffer:	   return "StorageBuffer";
			}

			ATN_CORE_ASSERT(false);
			return "";
		}

		static std::string_view ResourceTypeToString(ShaderResourceType type)
		{
			switch (type)
			{
			case ShaderResourceType::Texture2D:           return "Texture2D";
			case ShaderResourceType::TextureCube:         return "TextureCube";
			case ShaderResourceType::StorageTexture2D:    return "StorageTexture2D";
			case ShaderResourceType::StorageTextureCube:  return "StorageTextureCube";
			case ShaderResourceType::UniformBuffer:		  return "UniformBuffer";
			case ShaderResourceType::StorageBuffer:		  return "StorageBuffer";
			}

			ATN_CORE_ASSERT(false);
			return "";
		}

		static bool IsDescriptorImage(ShaderResourceType type)
		{
			switch (type)
			{
			case ShaderResourceType::Texture2D:       return true;
			case ShaderResourceType::TextureCube:     return true;
			case ShaderResourceType::StorageTexture2D:     return true;
			case ShaderResourceType::StorageTextureCube:   return true;
			case ShaderResourceType::UniformBuffer:	  return false;
			case ShaderResourceType::StorageBuffer:	  return false;
			}

			ATN_CORE_ASSERT(false);
			return false;
		}
	}

	DescriptorSetManager::DescriptorSetManager(const DescriptorSetManagerCreateInfo& info)
	{
		m_Info = info;
		m_ResourcesDescriptionTable = &m_Info.Shader->GetResourcesDescription();
		m_DescriptorPool = VK_NULL_HANDLE;

		// Fill in resources description table usaing shader meta data
		// And create write descriptor tables
		m_WriteDescriptorSetTable.resize(Renderer::GetFramesInFlight());

		for (const auto& [name, resourceDesc] : *m_ResourcesDescriptionTable)
		{
			if (IsValidSet(resourceDesc.Set))
			{
				WriteDescriptorSet& wd = m_WriteDescriptorSetTable[0][resourceDesc.Set][resourceDesc.Binding];
				wd.ResourcesState.resize(resourceDesc.ArraySize);

				VkWriteDescriptorSet& vulkanWD = wd.VulkanWriteDescriptorSet;
				vulkanWD.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				vulkanWD.dstBinding = resourceDesc.Binding;
				vulkanWD.descriptorType = Vulkan::GetDescriptorType(resourceDesc.Type);
				vulkanWD.dstArrayElement = 0;
				vulkanWD.descriptorCount = resourceDesc.ArraySize;

				RenderResourceStorage& resourceStorage = m_Resources[resourceDesc.Set][resourceDesc.Binding];
				resourceStorage.Type = resourceDesc.Type;
				resourceStorage.Storage.resize(resourceDesc.ArraySize);
				resourceStorage.IsDescriptorImageType = Utils::IsDescriptorImage(resourceStorage.Type);
			}
		}

		for (uint32 frameIndex = 1; frameIndex < Renderer::GetFramesInFlight(); ++frameIndex)
			m_WriteDescriptorSetTable[frameIndex] = m_WriteDescriptorSetTable[0];

		// Insert default textures for set 0 (for materials)
		for (auto& [set, setData] : m_Resources)
		{
			if (set > 0)
				continue;

			for (auto& [binding, resource] : setData)
			{
				if (resource.Type == ShaderResourceType::Texture2D || resource.Type == ShaderResourceType::StorageTexture2D)
				{
					for (uint32 i = 0; i < resource.Storage.size(); ++i)
						resource.Storage[i] = TextureGenerator::GetWhiteTexture();
				}
				else if(resource.Type == ShaderResourceType::TextureCube || resource.Type == ShaderResourceType::StorageTextureCube)
				{
					for (uint32 i = 0; i < resource.Storage.size(); ++i)
						resource.Storage[i] = TextureGenerator::GetBlackTextureCube();
				}
			}
		}
	}

	DescriptorSetManager::~DescriptorSetManager()
	{
		if (m_DescriptorPool != VK_NULL_HANDLE)
		{
			Renderer::SubmitResourceFree([pool = m_DescriptorPool]()
			{
				vkDestroyDescriptorPool(VulkanContext::GetLogicalDevice(), pool, nullptr);
			});
		}
	}

	void DescriptorSetManager::Set(const String& name, const Ref<RenderResource>& resource, uint32 arrayIndex)
	{
		RenderResourceStorage* storage = GetResourceStorage(name, arrayIndex);

		if (storage)
			storage->Storage[arrayIndex] = resource;
	}

	Ref<RenderResource> DescriptorSetManager::Get(const String& name, uint32 arrayIndex)
	{
		RenderResourceStorage* storage = GetResourceStorage(name, arrayIndex);

		if (storage)
			return storage->Storage[arrayIndex];

		return nullptr;
	}

	bool DescriptorSetManager::Validate() const
	{
		for (const auto& [name, resDesc] : *m_ResourcesDescriptionTable)
		{
			if (!IsValidSet(resDesc.Set))
				continue;

			if (!m_Resources.contains(resDesc.Set))
			{
				ATN_CORE_ERROR_TAG("Renderer", "DescriptorSetManager '{}' - No input resources for set {}", m_Info.Name, resDesc.Set);
				return false;
			}

			const auto& setResources = m_Resources.at(resDesc.Set);

			if (!setResources.contains(resDesc.Binding))
			{
				ATN_CORE_ERROR_TAG("Renderer", "DescriptorSetManager '{}' - No input resource '{}' for set {}, binding {}", m_Info.Name, name, resDesc.Set, resDesc.Binding);
				return false;
			}

			const auto& resource = setResources.at(resDesc.Binding);

			for (uint32 i = 0; i < resource.Storage.size(); ++i)
			{
				if (resource.Storage[i] == nullptr)
				{
					ATN_CORE_ERROR_TAG("Renderer", "DescriptorSetManager '{}' - Resource '{}' is NULL (set {}, binding {}, arrayIndex {})!",
						m_Info.Name, name, resDesc.Set, resDesc.Binding, i);
					return false;
				}
			}

			for (uint32 i = 0; i < resource.Storage.size(); ++i)
			{
				if (!IsCompatible(resource.Storage[i]->GetResourceType(), resDesc.Type))
				{
					ATN_CORE_ERROR_TAG("Renderer", "DescriptorSetManager '{}' - Required resource '{}' is wrong type (expected - '{}', given - '{}', set {}, binding {}, arrayIndex {})", 
						m_Info.Name, name, Utils::ResourceTypeToString(resDesc.Type), Utils::ResourceTypeToString(resource.Storage[i]->GetResourceType()), resDesc.Set, resDesc.Binding, i);
					return false;
				}
			}
		}

		return true;
	}

	void DescriptorSetManager::Bake()
	{
		if (!Validate())
		{
			ATN_CORE_ERROR_TAG("Renderer", "DescriptorSetManager '{}' - Validation has failed!", m_Info.Name);
			ATN_CORE_ASSERT(false);
			return;
		}

		// Calculate descriptor pool size and allocate it
		std::unordered_map<VkDescriptorType, uint32> poolSizesMap;
		uint32 maxSet = 0;

		for (const auto& [set, setData] : m_WriteDescriptorSetTable[0])
		{
			for (const auto& [binding, wd] : setData)
				poolSizesMap[wd.VulkanWriteDescriptorSet.descriptorType] += 1;

			if (set > maxSet)
				maxSet = set;
		}

		uint32 setsCount = maxSet - m_Info.FirstSet + 1;

		std::vector<VkDescriptorPoolSize> poolSizes;
		poolSizes.reserve(poolSizesMap.size());
		for (const auto& [type, count] : poolSizesMap)
			poolSizes.emplace_back(type, count);

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.maxSets = setsCount * Renderer::GetFramesInFlight();
		poolInfo.poolSizeCount = poolSizes.size();
		poolInfo.pPoolSizes = poolSizes.data();

		// Do not create pool if there are no resources
		if (poolInfo.maxSets > 0 && poolInfo.poolSizeCount > 0)
		{
			VK_CHECK(vkCreateDescriptorPool(VulkanContext::GetDevice()->GetLogicalDevice(), &poolInfo, nullptr, &m_DescriptorPool));
		}
		else
		{
			m_DescriptorPool = VK_NULL_HANDLE;
		}

		const auto& setLayouts = m_Info.Shader.As<VulkanShader>()->GetAllDescriptorSetLayouts();
		m_DescriptorSets.resize(Renderer::GetFramesInFlight());

		for (uint32 frameIndex = 0; frameIndex < Renderer::GetFramesInFlight(); ++frameIndex)
		{
			if (poolSizes.size() > 0)
			{
				VkDescriptorSetAllocateInfo allocInfo = {};
				allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				allocInfo.descriptorPool = m_DescriptorPool;
				allocInfo.descriptorSetCount = setsCount;
				allocInfo.pSetLayouts = &setLayouts[m_Info.FirstSet];

				m_DescriptorSets[frameIndex].resize(setsCount);
				VK_CHECK(vkAllocateDescriptorSets(VulkanContext::GetLogicalDevice(), &allocInfo, m_DescriptorSets[frameIndex].data()));

				for (uint32 i = 0; i < setsCount; ++i)
				{
					Vulkan::SetObjectDebugName(m_DescriptorSets[frameIndex][i], VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT,
						std::format("{}_{}_f{}", m_Info.Name, i, frameIndex));
				}
			}

			for (const auto& [set, setData] : m_Resources)
			{
				std::vector<std::vector<VkDescriptorImageInfo>> imageInfos;
				std::vector<std::vector<VkDescriptorBufferInfo>> bufferInfos;

				for (const auto& [binding, resource] : setData)
				{
					WriteDescriptorSet& storedWriteDescriptor = m_WriteDescriptorSetTable[frameIndex].at(set).at(binding);
					VkWriteDescriptorSet& writeDescriptor = storedWriteDescriptor.VulkanWriteDescriptorSet;
					VkDescriptorType descriptorType = writeDescriptor.descriptorType;

					writeDescriptor.dstSet = m_DescriptorSets[frameIndex].at(set - m_Info.FirstSet);

					if (resource.IsDescriptorImageType)
					{
						uint32 imageInfoIndex = imageInfos.size();
						imageInfos.emplace_back(resource.Storage.size());
						writeDescriptor.pImageInfo = imageInfos[imageInfoIndex].data();

						for (uint32 i = 0; i < resource.Storage.size(); ++i)
						{
							ResourceState& resourceState = storedWriteDescriptor.ResourcesState[i];
							VkDescriptorImageInfo& imageInfo = imageInfos[imageInfoIndex][i];
							imageInfo = GetDescriptorImage(resource.Storage[i]);

							// Force to have general layout if storage image only at bake(initialization)
							if (descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
								imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

							resourceState.ResourceHandle = imageInfo.imageView;
							resourceState.ImageLayout = imageInfo.imageLayout;
							resourceState.Sampler = imageInfo.sampler;

							if (resourceState.ResourceHandle == nullptr)
							{
								m_InvalidatedResources[set][binding] = resource;
								break;
							}
						}
					}
					else
					{
						uint32 bufferInfoIndex = bufferInfos.size();
						bufferInfos.emplace_back(resource.Storage.size());
						writeDescriptor.pBufferInfo = bufferInfos[bufferInfoIndex].data();

						for (uint32 i = 0; i < resource.Storage.size(); ++i)
						{
							ResourceState& resourceState = storedWriteDescriptor.ResourcesState[i];
							VkDescriptorBufferInfo& bufferInfo = bufferInfos[bufferInfoIndex][i];
							bufferInfo = GetDescriptorBuffer(resource.Storage[i], frameIndex);

							resourceState.ResourceHandle = bufferInfo.buffer;

							if (resourceState.ResourceHandle == nullptr)
							{
								m_InvalidatedResources[set][binding] = resource;
								break;
							}
						}
					}
				}

				std::vector<VkWriteDescriptorSet> descriptorsToUpdate;
				for (const auto& [binding, writeDescriptor] : m_WriteDescriptorSetTable[frameIndex].at(set))
				{
					// update if valid, otherwise defer (will be updated at rendering stage)
					if (!IsInvalidated(set, binding))
						descriptorsToUpdate.emplace_back(writeDescriptor.VulkanWriteDescriptorSet);
				}

				if (!descriptorsToUpdate.empty())
				{
					//ATN_CORE_TRACE_TAG("Renderer", "DescriptorSetManager '{}' - Bake Updating descriptors in set {} (frameIndex {})", m_Info.Name, set, frameIndex);
					vkUpdateDescriptorSets(VulkanContext::GetLogicalDevice(), descriptorsToUpdate.size(), descriptorsToUpdate.data(), 0, nullptr);
				}
			}
		}
	}

	void DescriptorSetManager::InvalidateAndUpdate()
	{
		if (m_DescriptorSets.empty())
			return;

		ATN_PROFILE_FUNC();

		uint32 frameIndex = Renderer::GetCurrentFrameIndex();

		// For each resource check if it is reseted or its state changed
		// if yes - save and update descriptor sets

		for (const auto& [set, setData] : m_Resources)
		{
			for (const auto& [binding, resource] : setData)
			{
				WriteDescriptorSet& wd = m_WriteDescriptorSetTable[frameIndex].at(set).at(binding);

				if (resource.IsDescriptorImageType)
				{
					for (uint32 i = 0; i < resource.Storage.size(); ++i)
					{
						const auto& imageInfo = GetDescriptorImage(resource.Storage[i]);
						if (IsResourceStateChanged(wd.ResourcesState[i], imageInfo))
						{
							m_InvalidatedResources[set][binding] = resource;
							break;
						}
					}
				}
				else
				{
					for (uint32 i = 0; i < resource.Storage.size(); ++i)
					{
						const auto& bufferInfo = GetDescriptorBuffer(resource.Storage[i], frameIndex);
						if (IsResourceStateChanged(wd.ResourcesState[i], bufferInfo))
						{
							m_InvalidatedResources[set][binding] = resource;
							break;
						}
					}
				}
			}
		}

		if (m_InvalidatedResources.empty())
			return;

		// Go through every invalidated resource and update its corresponding write descriptor and descriptor set
		// If there is no valid buffer/texture, that is an error and will probably crash
		// TODO: handle errors
		for (const auto& [set, setData] : m_InvalidatedResources)
		{
			std::vector<VkWriteDescriptorSet> descriptorsToUpdate;
			descriptorsToUpdate.reserve(setData.size());

			std::vector<std::vector<VkDescriptorImageInfo>> imageInfos;
			std::vector<std::vector<VkDescriptorBufferInfo>> bufferInfos;

			for (const auto& [binding, resource] : setData)
			{
				WriteDescriptorSet& storedWriteDescriptor = m_WriteDescriptorSetTable[frameIndex].at(set).at(binding);
				VkWriteDescriptorSet& writeDescriptor = storedWriteDescriptor.VulkanWriteDescriptorSet;

				if (resource.IsDescriptorImageType)
				{
					uint32 imageInfoIndex = imageInfos.size();
					imageInfos.emplace_back(resource.Storage.size());
					writeDescriptor.pImageInfo = imageInfos[imageInfoIndex].data();

					for (uint32 i = 0; i < resource.Storage.size(); ++i)
					{
						ResourceState& resourceState = storedWriteDescriptor.ResourcesState[i];
						VkDescriptorImageInfo& imageInfo = imageInfos[imageInfoIndex][i];
						imageInfo = GetDescriptorImage(resource.Storage[i]);

						resourceState.ResourceHandle = imageInfo.imageView;
						resourceState.ImageLayout = imageInfo.imageLayout;
						resourceState.Sampler = imageInfo.sampler;
					}
				}
				else
				{
					uint32 bufferInfoIndex = bufferInfos.size();
					bufferInfos.emplace_back(resource.Storage.size());
					writeDescriptor.pBufferInfo = bufferInfos[bufferInfoIndex].data();

					for (uint32 i = 0; i < resource.Storage.size(); ++i)
					{
						ResourceState& resourceState = storedWriteDescriptor.ResourcesState[i];
						VkDescriptorBufferInfo& bufferInfo = bufferInfos[bufferInfoIndex][i];
						bufferInfo = GetDescriptorBuffer(resource.Storage[i], frameIndex);

						resourceState.ResourceHandle = bufferInfo.buffer;
					}
				}

				descriptorsToUpdate.push_back(writeDescriptor);
			}

			if (!descriptorsToUpdate.empty())
			{
				//ATN_CORE_TRACE_TAG("Renderer", "Updating descriptor set '{}' (set = {}, frameIndex = {})", m_Info.Name, set, frameIndex);
				vkUpdateDescriptorSets(VulkanContext::GetLogicalDevice(), descriptorsToUpdate.size(), descriptorsToUpdate.data(), 0, nullptr);
			}
		}

		m_InvalidatedResources.clear();
	}

	void DescriptorSetManager::BindDescriptorSets(VkCommandBuffer vkcommandBuffer, VkPipelineBindPoint bindPoint)
	{
		if (m_DescriptorSets.size() == 0)
			return;

		const auto& descriptorSets = m_DescriptorSets[Renderer::GetCurrentFrameIndex()];

		if (descriptorSets.size() > 0)
		{
			vkCmdBindDescriptorSets(
				vkcommandBuffer,
				bindPoint,
				m_Info.Shader.As<VulkanShader>()->GetPipelineLayout(),
				m_Info.FirstSet, descriptorSets.size(),
				&descriptorSets[0],
				0, 0);
		}
	}

	bool DescriptorSetManager::IsInvalidated(uint32 set, uint32 binding)
	{
		if (m_InvalidatedResources.contains(set))
		{
			if (m_InvalidatedResources.at(set).contains(binding))
			{
				return true;
			}
		}

		return false;
	}

	RenderResourceStorage* DescriptorSetManager::GetResourceStorage(const String& name, uint32 arrayIndex)
	{
		if (m_ResourcesDescriptionTable->contains(name))
		{
			const ShaderResourceDescription& desc = m_ResourcesDescriptionTable->at(name);

			if (desc.Set >= m_Info.FirstSet && desc.Set <= m_Info.LastSet)
			{
				RenderResourceStorage& storage = m_Resources.at(desc.Set).at(desc.Binding);
				if (arrayIndex < storage.Storage.size())
				{
					return &storage;
				}
				else
				{
					ATN_CORE_ERROR_TAG("Renderer", "DescriptorSetManager '{}' - Failed to get or set resource with name '{}' (arrayIndex is too big, given - '{}', max - '{}')",
						m_Info.Name, name, arrayIndex, storage.Storage.size());
				}
			}
			else
			{
				ATN_CORE_ERROR_TAG("Renderer", "DescriptorSetManager '{}' - Failed to get or set resource with name '{}' (invalid name)", m_Info.Name, name);
			}
		}
		else
		{
			ATN_CORE_ERROR_TAG("Renderer", "DescriptorSetManager '{}' - Failed to get or set resource with name '{}' (invalid name)", m_Info.Name, name);
		}

		return nullptr;
	}

	const VkDescriptorImageInfo& DescriptorSetManager::GetDescriptorImage(const Ref<RenderResource>& resource)
	{
		switch (resource->GetResourceType())
		{
		case RenderResourceType::Texture2D:    return resource.As<VulkanTexture2D>()->GetVulkanDescriptorInfo();
		case RenderResourceType::TextureCube:  return resource.As<VulkanTextureCube>()->GetVulkanDescriptorInfo();
		case RenderResourceType::TextureView2D:  return resource.As<VulkanTextureView>()->GetVulkanDescriptorInfo();
		case RenderResourceType::TextureViewCube:  return resource.As<VulkanTextureView>()->GetVulkanDescriptorInfo();
		}

		ATN_CORE_ASSERT(false);
		return resource.As<VulkanTexture2D>()->GetVulkanDescriptorInfo();
	}

	const VkDescriptorBufferInfo& DescriptorSetManager::GetDescriptorBuffer(const Ref<RenderResource>& resource, uint32 frameIndex)
	{
		switch (resource->GetResourceType())
		{
		case RenderResourceType::UniformBuffer:  return resource.As<VulkanUniformBuffer>()->GetVulkanDescriptorInfo(frameIndex);
		case RenderResourceType::StorageBuffer:  return resource.As<VulkanStorageBuffer>()->GetVulkanDescriptorInfo(frameIndex);
		}

		ATN_CORE_ASSERT(false);
		return resource.As<VulkanUniformBuffer>()->GetVulkanDescriptorInfo(frameIndex);
	}

	bool DescriptorSetManager::IsCompatible(RenderResourceType renderType, ShaderResourceType shaderType) const
	{
		if (renderType == RenderResourceType::UniformBuffer && shaderType == ShaderResourceType::UniformBuffer)
			return true;

		if (renderType == RenderResourceType::StorageBuffer && shaderType == ShaderResourceType::StorageBuffer)
			return true;

		bool renderTexture2D = false;
		switch (renderType)
		{
		case RenderResourceType::Texture2D: renderTexture2D = true; break;
		case RenderResourceType::TextureView2D: renderTexture2D = true; break;
		}

		bool shaderTexture2D = false;
		switch (shaderType)
		{
		case ShaderResourceType::Texture2D: shaderTexture2D = true; break;
		case ShaderResourceType::StorageTexture2D: shaderTexture2D = true; break;
		}

		if (renderTexture2D != shaderTexture2D)
			return false;

		if (renderTexture2D && shaderTexture2D)
			return true;

		bool renderTextureCube = false;
		switch (renderType)
		{
		case RenderResourceType::TextureCube: renderTextureCube = true; break;
		case RenderResourceType::TextureViewCube: renderTextureCube = true; break;
		}

		bool shaderTextureCube = false;
		switch (shaderType)
		{
		case ShaderResourceType::TextureCube: shaderTextureCube = true; break;
		case ShaderResourceType::StorageTextureCube: shaderTextureCube = true; break;
		}

		if (renderTextureCube != shaderTextureCube)
			return false;

		if (renderTextureCube && shaderTextureCube)
			return true;

		return false;
	}

	bool DescriptorSetManager::IsValidSet(uint32 set) const
	{
		return set >= m_Info.FirstSet && set <= m_Info.LastSet;
	}

	bool DescriptorSetManager::IsResourceStateChanged(const ResourceState& oldState, const VkDescriptorImageInfo& imageInfo)
	{
		return !(oldState.ResourceHandle == imageInfo.imageView && oldState.ImageLayout == imageInfo.imageLayout && oldState.Sampler == imageInfo.sampler);
	}

	bool DescriptorSetManager::IsResourceStateChanged(const ResourceState& oldState, const VkDescriptorBufferInfo& bufferInfo)
	{
		return oldState.ResourceHandle != bufferInfo.buffer;
	}
}
