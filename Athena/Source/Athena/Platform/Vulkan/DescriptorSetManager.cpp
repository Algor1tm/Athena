#include "DescriptorSetManager.h"
#include "Athena/Platform/Vulkan/VulkanTexture2D.h"
#include "Athena/Platform/Vulkan/VulkanTextureCube.h"
#include "Athena/Platform/Vulkan/VulkanUniformBuffer.h"
#include "Athena/Platform/Vulkan/VulkanStorageBuffer.h"
#include "Athena/Platform/Vulkan/VulkanShader.h"
#include "Athena/Platform/Vulkan/VulkanUtils.h"


namespace Athena
{
	namespace Vulkan
	{
		static VkDescriptorType GetDescriptorType(ShaderResourceType type)
		{
			switch (type)
			{
			case ShaderResourceType::Texture2D:           return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			case ShaderResourceType::RWTexture2D:		  return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			case ShaderResourceType::TextureCube:         return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			case ShaderResourceType::RWTextureCube:       return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			case ShaderResourceType::UniformBuffer:	      return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			case ShaderResourceType::StorageBuffer:	      return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			}

			ATN_CORE_ASSERT(false);
			return (VkDescriptorType)0;
		}
	}

	namespace Utils
	{
		static RenderResourceType ShaderResourceToRenderResource(ShaderResourceType type)
		{
			switch (type)
			{
			case ShaderResourceType::Texture2D:           return RenderResourceType::Texture2D;
			case ShaderResourceType::TextureCube:         return RenderResourceType::TextureCube;
			case ShaderResourceType::RWTexture2D:         return RenderResourceType::Texture2D;
			case ShaderResourceType::RWTextureCube:       return RenderResourceType::TextureCube;
			case ShaderResourceType::UniformBuffer:	      return RenderResourceType::UniformBuffer;
			case ShaderResourceType::StorageBuffer:	      return RenderResourceType::StorageBuffer;
			}

			ATN_CORE_ASSERT(false);
			return (RenderResourceType)0;
		}

		static std::string_view ResourceTypeToString(RenderResourceType type)
		{
			switch(type)
			{
			case RenderResourceType::Texture2D:      return "Texture2D";
			case RenderResourceType::TextureCube:    return "TextureCube";
			case RenderResourceType::UniformBuffer:	 return "UniformBuffer";
			case RenderResourceType::StorageBuffer:	 return "StorageBuffer";
			}

			ATN_CORE_ASSERT(false);
			return "";
		}

		static std::string_view ResourceTypeToString(ShaderResourceType type)
		{
			return ResourceTypeToString(Utils::ShaderResourceToRenderResource(type));
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
				resourceStorage.Type = Utils::ShaderResourceToRenderResource(resourceDesc.Type);
				resourceStorage.Storage.resize(resourceDesc.ArraySize);
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
				if (resource.Type == RenderResourceType::Texture2D)
				{
					for (uint32 i = 0; i < resource.Storage.size(); ++i)
						resource.Storage[i] = Renderer::GetWhiteTexture();
				}
				else if(resource.Type == RenderResourceType::TextureCube)
				{
					for (uint32 i = 0; i < resource.Storage.size(); ++i)
						resource.Storage[i] = Renderer::GetBlackTextureCube();
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

	void DescriptorSetManager::Set(const String& name, const Ref<Texture>& resource, uint32 arrayIndex, uint32 mip)
	{
		RenderResourceStorage* storage = GetResourceStorage(name, arrayIndex);

		if (storage)
		{
			storage->Storage[arrayIndex] = resource;
			storage->MipLevel = mip;
		}
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

					switch (resource.Type)
					{
					case RenderResourceType::Texture2D:
					{
						uint32 imageInfoIndex = imageInfos.size();
						imageInfos.emplace_back(resource.Storage.size());
						writeDescriptor.pImageInfo = imageInfos[imageInfoIndex].data();

						for (uint32 i = 0; i < resource.Storage.size(); ++i)
						{
							ResourceState& resourceState = storedWriteDescriptor.ResourcesState[i];
							VkDescriptorImageInfo& imageInfo = imageInfos[imageInfoIndex][i];
							imageInfo = resource.Storage[i].As<VulkanTexture2D>()->GetVulkanDescriptorInfo(resource.MipLevel);

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
						break;
					}
					case RenderResourceType::TextureCube:
					{
						uint32 imageInfoIndex = imageInfos.size();
						imageInfos.emplace_back(resource.Storage.size());
						writeDescriptor.pImageInfo = imageInfos[imageInfoIndex].data();

						for (uint32 i = 0; i < resource.Storage.size(); ++i)
						{
							ResourceState& resourceState = storedWriteDescriptor.ResourcesState[i];
							VkDescriptorImageInfo& imageInfo = imageInfos[imageInfoIndex][i];
							imageInfo = resource.Storage[i].As<VulkanTextureCube>()->GetVulkanDescriptorInfo(resource.MipLevel);

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
						break;
					}
					case RenderResourceType::UniformBuffer:
					{
						uint32 bufferInfoIndex = bufferInfos.size();
						bufferInfos.emplace_back(resource.Storage.size());
						writeDescriptor.pBufferInfo = bufferInfos[bufferInfoIndex].data();

						for (uint32 i = 0; i < resource.Storage.size(); ++i)
						{
							ResourceState& resourceState = storedWriteDescriptor.ResourcesState[i];
							VkDescriptorBufferInfo& bufferInfo = bufferInfos[bufferInfoIndex][i];
							bufferInfo = resource.Storage[i].As<VulkanUniformBuffer>()->GetVulkanDescriptorInfo(frameIndex);

							resourceState.ResourceHandle = bufferInfo.buffer;

							if (resourceState.ResourceHandle == nullptr)
							{
								m_InvalidatedResources[set][binding] = resource;
								break;
							}
						}
						break;
					}
					case RenderResourceType::StorageBuffer:
					{
						uint32 bufferInfoIndex = bufferInfos.size();
						bufferInfos.emplace_back(resource.Storage.size());
						writeDescriptor.pBufferInfo = bufferInfos[bufferInfoIndex].data();

						for (uint32 i = 0; i < resource.Storage.size(); ++i)
						{
							ResourceState& resourceState = storedWriteDescriptor.ResourcesState[i];
							VkDescriptorBufferInfo& bufferInfo = bufferInfos[bufferInfoIndex][i];
							bufferInfo = resource.Storage[i].As<VulkanStorageBuffer>()->GetVulkanDescriptorInfo(frameIndex);

							resourceState.ResourceHandle = bufferInfo.buffer;

							if (resourceState.ResourceHandle == nullptr)
							{
								m_InvalidatedResources[set][binding] = resource;
								break;
							}
						}
						break;
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
					ATN_CORE_INFO_TAG("Renderer", "DescriptorSetManager '{}' - Bake Updating descriptors in set {} (frameIndex {})", m_Info.Name, set, frameIndex);
					vkUpdateDescriptorSets(VulkanContext::GetLogicalDevice(), descriptorsToUpdate.size(), descriptorsToUpdate.data(), 0, nullptr);
				}
			}
		}
	}

	void DescriptorSetManager::RT_InvalidateAndUpdate()
	{
		if (m_DescriptorSets.size() == 0)
			return;

		uint32 frameIndex = Renderer::GetCurrentFrameIndex();

		// For each resource check if it is reseted or its state changed
		// if yes - save and update descriptor sets

		for (const auto& [set, setData] : m_Resources)
		{
			for (const auto& [binding, resource] : setData)
			{
				WriteDescriptorSet& wd = m_WriteDescriptorSetTable[frameIndex].at(set).at(binding);

				switch (resource.Type)
				{
				case RenderResourceType::Texture2D:
				{
					for (uint32 i = 0; i < resource.Storage.size(); ++i)
					{
						const auto& imageInfo = resource.Storage[i].As<VulkanTexture2D>()->GetVulkanDescriptorInfo(resource.MipLevel);
						if (IsResourceStateChanged(wd.ResourcesState[i], imageInfo))
						{
							m_InvalidatedResources[set][binding] = resource;
							break;
						}
					}

					break;
				}
				case RenderResourceType::TextureCube:
				{
					for (uint32 i = 0; i < resource.Storage.size(); ++i)
					{
						const auto& imageInfo = resource.Storage[i].As<VulkanTextureCube>()->GetVulkanDescriptorInfo(resource.MipLevel);
						if (IsResourceStateChanged(wd.ResourcesState[i], imageInfo))
						{
							m_InvalidatedResources[set][binding] = resource;
							break;
						}
					}
					break;
				}
				case RenderResourceType::UniformBuffer:
				{
					for (uint32 i = 0; i < resource.Storage.size(); ++i)
					{
						const auto& bufferInfo = resource.Storage[i].As<VulkanUniformBuffer>()->GetVulkanDescriptorInfo(frameIndex);
						if (IsResourceStateChanged(wd.ResourcesState[i], bufferInfo))
						{
							m_InvalidatedResources[set][binding] = resource;
						}
					}
					break;
				}
				case RenderResourceType::StorageBuffer:
				{
					for (uint32 i = 0; i < resource.Storage.size(); ++i)
					{
						const auto& bufferInfo = resource.Storage[i].As<VulkanStorageBuffer>()->GetVulkanDescriptorInfo(frameIndex);
						if (IsResourceStateChanged(wd.ResourcesState[i], bufferInfo))
						{
							m_InvalidatedResources[set][binding] = resource;
							break;
						}
					}
					break;
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

				switch (resource.Type)
				{
				case RenderResourceType::Texture2D:
				{
					uint32 imageInfoIndex = imageInfos.size();
					imageInfos.emplace_back(resource.Storage.size());
					writeDescriptor.pImageInfo = imageInfos[imageInfoIndex].data();

					for (uint32 i = 0; i < resource.Storage.size(); ++i)
					{
						ResourceState& resourceState = storedWriteDescriptor.ResourcesState[i];
						VkDescriptorImageInfo& imageInfo = imageInfos[imageInfoIndex][i];
						imageInfo = resource.Storage[i].As<VulkanTexture2D>()->GetVulkanDescriptorInfo(resource.MipLevel);

						resourceState.ResourceHandle = imageInfo.imageView;
						resourceState.ImageLayout = imageInfo.imageLayout;
						resourceState.Sampler = imageInfo.sampler;
					}
					break;
				}
				case RenderResourceType::TextureCube:
				{
					uint32 imageInfoIndex = imageInfos.size();
					imageInfos.emplace_back(resource.Storage.size());
					writeDescriptor.pImageInfo = imageInfos[imageInfoIndex].data();

					for (uint32 i = 0; i < resource.Storage.size(); ++i)
					{
						ResourceState& resourceState = storedWriteDescriptor.ResourcesState[i];
						VkDescriptorImageInfo& imageInfo = imageInfos[imageInfoIndex][i];
						imageInfo = resource.Storage[i].As<VulkanTextureCube>()->GetVulkanDescriptorInfo(resource.MipLevel);

						resourceState.ResourceHandle = imageInfo.imageView;
						resourceState.ImageLayout = imageInfo.imageLayout;
						resourceState.Sampler = imageInfo.sampler;
					}
					break;
				}
				case RenderResourceType::UniformBuffer:
				{
					uint32 bufferInfoIndex = bufferInfos.size();
					bufferInfos.emplace_back(resource.Storage.size());
					writeDescriptor.pBufferInfo = bufferInfos[bufferInfoIndex].data();

					for (uint32 i = 0; i < resource.Storage.size(); ++i)
					{
						ResourceState& resourceState = storedWriteDescriptor.ResourcesState[i];
						VkDescriptorBufferInfo& bufferInfo = bufferInfos[bufferInfoIndex][i];
						bufferInfo = resource.Storage[i].As<VulkanUniformBuffer>()->GetVulkanDescriptorInfo(frameIndex);

						resourceState.ResourceHandle = bufferInfo.buffer;
					}
					break;
				}
				case RenderResourceType::StorageBuffer:
				{
					uint32 bufferInfoIndex = bufferInfos.size();
					bufferInfos.emplace_back(resource.Storage.size());
					writeDescriptor.pBufferInfo = bufferInfos[bufferInfoIndex].data();

					for (uint32 i = 0; i < resource.Storage.size(); ++i)
					{
						ResourceState& resourceState = storedWriteDescriptor.ResourcesState[i];
						VkDescriptorBufferInfo& bufferInfo = bufferInfos[bufferInfoIndex][i];
						bufferInfo = resource.Storage[i].As<VulkanStorageBuffer>()->GetVulkanDescriptorInfo(frameIndex);

						resourceState.ResourceHandle = bufferInfo.buffer;
					}
					break;
				}
				}

				descriptorsToUpdate.push_back(writeDescriptor);
			}

			if (!descriptorsToUpdate.empty())
			{
				ATN_CORE_INFO_TAG("Renderer", "DescriptorSetManager '{}' - Updating descriptors in set {} (frameIndex {})", m_Info.Name, set, frameIndex);
				vkUpdateDescriptorSets(VulkanContext::GetLogicalDevice(), descriptorsToUpdate.size(), descriptorsToUpdate.data(), 0, nullptr);
			}
		}

		m_InvalidatedResources.clear();
	}

	void DescriptorSetManager::RT_BindDescriptorSets(VkCommandBuffer vkcommandBuffer, VkPipelineBindPoint bindPoint)
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

	bool DescriptorSetManager::IsCompatible(RenderResourceType renderType, ShaderResourceType shaderType) const
	{
		return Utils::ShaderResourceToRenderResource(shaderType) == renderType;
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
