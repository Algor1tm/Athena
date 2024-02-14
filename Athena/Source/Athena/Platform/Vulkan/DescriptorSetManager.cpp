#include "DescriptorSetManager.h"
#include "Athena/Platform/Vulkan/VulkanTexture2D.h"
#include "Athena/Platform/Vulkan/VulkanTextureCube.h"
#include "Athena/Platform/Vulkan/VulkanUniformBuffer.h"
#include "Athena/Platform/Vulkan/VulkanStorageBuffer.h"
#include "Athena/Platform/Vulkan/VulkanShader.h"
#include "Athena/Platform/Vulkan/VulkanUtils.h"


namespace Athena
{
	namespace Utils
	{
		static std::string_view ResourceTypeToString(ShaderResourceType type)
		{
			switch(type)
			{
			case ShaderResourceType::Texture2D:          return "Texture2D";
			case ShaderResourceType::TextureCube:        return "TextureCube";
			case ShaderResourceType::UniformBuffer:		 return "UniformBuffer";
			case ShaderResourceType::StorageBuffer:		 return "StorageBuffer";
			}

			ATN_CORE_ASSERT(false);
			return "";
		}
	}

	DescriptorSetManager::DescriptorSetManager(const DescriptorSetManagerCreateInfo& info)
	{
		m_Info = info;

		// Fill in resources description table usaing shader meta data
		// And create write descriptor table
		m_WriteDescriptorSetTable.resize(Renderer::GetFramesInFlight());
		const ShaderMetaData& metaData = m_Info.Shader->GetMetaData();

		for (const auto& [name, texture] : metaData.SampledTextures)
		{
			if (texture.Set >= m_Info.FirstSet && texture.Set <= m_Info.LastSet)
			{
				ShaderResourceDescription resourceDesc;
				resourceDesc.Type = texture.ImageType == ImageType::IMAGE_2D ? ShaderResourceType::Texture2D : ShaderResourceType::TextureCube;
				resourceDesc.Binding = texture.Binding;
				resourceDesc.Set = texture.Set;
				resourceDesc.ArraySize = texture.ArraySize;
				m_ResourcesDescriptionTable[name] = resourceDesc;

				WriteDescriptorSet& wd = m_WriteDescriptorSetTable[0][resourceDesc.Set][resourceDesc.Binding];
				wd.ResourcesState.resize(texture.ArraySize);

				VkWriteDescriptorSet& vulkanWD = wd.VulkanWriteDescriptorSet;
				vulkanWD.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				vulkanWD.dstBinding = resourceDesc.Binding;
				vulkanWD.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				vulkanWD.dstArrayElement = 0;
				vulkanWD.descriptorCount = texture.ArraySize;

				m_Resources[texture.Set][texture.Binding].Type = resourceDesc.Type;
				m_Resources.at(texture.Set).at(texture.Binding).Storage.resize(texture.ArraySize);
			}
		}
		for (const auto& [name, texture] : metaData.StorageTextures)
		{
			if (texture.Set >= m_Info.FirstSet && texture.Set <= m_Info.LastSet)
			{
				ShaderResourceDescription resourceDesc;
				resourceDesc.Type = texture.ImageType == ImageType::IMAGE_2D ? ShaderResourceType::Texture2D : ShaderResourceType::TextureCube;
				resourceDesc.Binding = texture.Binding;
				resourceDesc.Set = texture.Set;
				resourceDesc.ArraySize = texture.ArraySize;
				m_ResourcesDescriptionTable[name] = resourceDesc;

				WriteDescriptorSet& wd = m_WriteDescriptorSetTable[0][resourceDesc.Set][resourceDesc.Binding];
				wd.ResourcesState.resize(texture.ArraySize);

				VkWriteDescriptorSet& vulkanWD = wd.VulkanWriteDescriptorSet;
				vulkanWD.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				vulkanWD.dstBinding = resourceDesc.Binding;
				vulkanWD.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				vulkanWD.dstArrayElement = 0;
				vulkanWD.descriptorCount = texture.ArraySize;

				m_Resources[texture.Set][texture.Binding].Type = resourceDesc.Type;
				m_Resources.at(texture.Set).at(texture.Binding).Storage.resize(texture.ArraySize);
			}
		}
		for (const auto& [name, buffer] : metaData.UniformBuffers)
		{
			if (buffer.Set >= m_Info.FirstSet && buffer.Set <= m_Info.LastSet)
			{
				ShaderResourceDescription resourceDesc;
				resourceDesc.Type = ShaderResourceType::UniformBuffer;
				resourceDesc.Binding = buffer.Binding;
				resourceDesc.Set = buffer.Set;
				resourceDesc.ArraySize = buffer.ArraySize;
				m_ResourcesDescriptionTable[name] = resourceDesc;

				WriteDescriptorSet& wd = m_WriteDescriptorSetTable[0][resourceDesc.Set][resourceDesc.Binding];
				wd.ResourcesState.resize(buffer.ArraySize);

				VkWriteDescriptorSet& vulkanWD = wd.VulkanWriteDescriptorSet;
				vulkanWD.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				vulkanWD.dstBinding = resourceDesc.Binding;
				vulkanWD.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				vulkanWD.dstArrayElement = 0;
				vulkanWD.descriptorCount = buffer.ArraySize;

				m_Resources[buffer.Set][buffer.Binding].Type = resourceDesc.Type;
				m_Resources.at(buffer.Set).at(buffer.Binding).Storage.resize(buffer.ArraySize);
			}
		}
		for (const auto& [name, buffer] : metaData.StorageBuffers)
		{
			if (buffer.Set >= m_Info.FirstSet && buffer.Set <= m_Info.LastSet)
			{
				ShaderResourceDescription resourceDesc;
				resourceDesc.Type = ShaderResourceType::StorageBuffer;
				resourceDesc.Binding = buffer.Binding;
				resourceDesc.Set = buffer.Set;
				resourceDesc.ArraySize = buffer.ArraySize;
				m_ResourcesDescriptionTable[name] = resourceDesc;

				WriteDescriptorSet& wd = m_WriteDescriptorSetTable[0][resourceDesc.Set][resourceDesc.Binding];
				wd.ResourcesState.resize(buffer.ArraySize);

				VkWriteDescriptorSet& vulkanWD = wd.VulkanWriteDescriptorSet;
				vulkanWD.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				vulkanWD.dstBinding = resourceDesc.Binding;
				vulkanWD.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				vulkanWD.dstArrayElement = 0;
				vulkanWD.descriptorCount = buffer.ArraySize;

				m_Resources[buffer.Set][buffer.Binding].Type = resourceDesc.Type;
				m_Resources.at(buffer.Set).at(buffer.Binding).Storage.resize(buffer.ArraySize);
			}
		}

		for (uint32 frameIndex = 1; frameIndex < Renderer::GetFramesInFlight(); ++frameIndex)
			m_WriteDescriptorSetTable[frameIndex] = m_WriteDescriptorSetTable[0];

		// Insert default textures for set 0 (for materials)
		for (const auto& [name, resDesc] : m_ResourcesDescriptionTable)
		{
			if (resDesc.Type == ShaderResourceType::Texture2D && resDesc.Set == 0)
			{
				for(uint32 i = 0; i < resDesc.ArraySize; ++i)
					m_Resources[resDesc.Set][resDesc.Binding].Storage[i] = Renderer::GetWhiteTexture();
			}
		}
	}

	DescriptorSetManager::~DescriptorSetManager()
	{
		Renderer::SubmitResourceFree([pool = m_DescriptorPool]()
		{
			if(pool != VK_NULL_HANDLE)
				vkDestroyDescriptorPool(VulkanContext::GetLogicalDevice(), pool, nullptr);
		});
	}

	void DescriptorSetManager::Set(const String& name, const Ref<ShaderResource>& resource, uint32 arrayIndex)
	{
		if (m_ResourcesDescriptionTable.contains(name) && resource != nullptr)
		{
			const ShaderResourceDescription& desc = m_ResourcesDescriptionTable.at(name);
			m_Resources[desc.Set][desc.Binding].Storage[arrayIndex] = resource;
		}
		else
		{
			ATN_CORE_WARN_TAG("Renderer", "DescriptorSetManager '{}' - Failed to set shader resource with name '{}'", m_Info.Name, name);
		}
	}

	Ref<ShaderResource> DescriptorSetManager::Get(const String& name, uint32 arrayIndex)
	{
		if (m_ResourcesDescriptionTable.contains(name))
		{
			const ShaderResourceDescription& desc = m_ResourcesDescriptionTable.at(name);

			if (m_Resources.contains(desc.Set) && m_Resources.at(desc.Set).contains(desc.Binding))
			{
				const auto& resourceStorage = m_Resources.at(desc.Set).at(desc.Binding);;
				if (arrayIndex < resourceStorage.Storage.size())
					return resourceStorage.Storage[arrayIndex];
			}

			ATN_CORE_ERROR_TAG("Renderer", "DescriptorSetManager '{}' - Failed to get shader resource with name '{}' (resource is not present)", m_Info.Name, name);
		}
		else
		{
			ATN_CORE_ERROR_TAG("Renderer", "DescriptorSetManager '{}' - Failed to get shader resource with name '{}' (invalid name)", m_Info.Name, name);
		}

		return nullptr;
	}

	bool DescriptorSetManager::Validate() const
	{
		for (const auto& [name, resDesc] : m_ResourcesDescriptionTable)
		{
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
				if (resDesc.Type != resource.Storage[i]->GetResourceType())
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

					writeDescriptor.dstSet = m_DescriptorSets[frameIndex].at(set - m_Info.FirstSet);

					switch (resource.Type)
					{
					case ShaderResourceType::Texture2D:
					{
						uint32 imageInfoIndex = imageInfos.size();
						imageInfos.emplace_back(resource.Storage.size());

						for (uint32 i = 0; i < resource.Storage.size(); ++i)
						{
							ResourceState& resourceState = storedWriteDescriptor.ResourcesState[i];
							VkDescriptorImageInfo& imageInfo = imageInfos[imageInfoIndex][i];
							imageInfo = resource.Storage[i].As<VulkanTexture2D>()->GetVulkanDescriptorInfo();

							writeDescriptor.pImageInfo = imageInfos[imageInfoIndex].data();
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
					case ShaderResourceType::TextureCube:
					{
						uint32 imageInfoIndex = imageInfos.size();
						imageInfos.emplace_back(resource.Storage.size());

						for (uint32 i = 0; i < resource.Storage.size(); ++i)
						{
							ResourceState& resourceState = storedWriteDescriptor.ResourcesState[i];
							VkDescriptorImageInfo& imageInfo = imageInfos[imageInfoIndex][i];
							imageInfo = resource.Storage[i].As<VulkanTextureCube>()->GetVulkanDescriptorInfo();

							writeDescriptor.pImageInfo = imageInfos[imageInfoIndex].data();
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
					case ShaderResourceType::UniformBuffer:
					{
						uint32 bufferInfoIndex = bufferInfos.size();
						bufferInfos.emplace_back(resource.Storage.size());

						for (uint32 i = 0; i < resource.Storage.size(); ++i)
						{
							ResourceState& resourceState = storedWriteDescriptor.ResourcesState[i];
							VkDescriptorBufferInfo& bufferInfo = bufferInfos[bufferInfoIndex][i];
							bufferInfo = resource.Storage[i].As<VulkanUniformBuffer>()->GetVulkanDescriptorInfo(frameIndex);

							writeDescriptor.pBufferInfo = bufferInfos[bufferInfoIndex].data();
							resourceState.ResourceHandle = bufferInfo.buffer;

							if (resourceState.ResourceHandle == nullptr)
							{
								m_InvalidatedResources[set][binding] = resource;
								break;
							}
						}
						break;
					}
					case ShaderResourceType::StorageBuffer:
					{
						uint32 bufferInfoIndex = bufferInfos.size();
						bufferInfos.emplace_back(resource.Storage.size());

						for (uint32 i = 0; i < resource.Storage.size(); ++i)
						{
							ResourceState& resourceState = storedWriteDescriptor.ResourcesState[i];
							VkDescriptorBufferInfo& bufferInfo = bufferInfos[bufferInfoIndex][i];
							bufferInfo = resource.Storage[i].As<VulkanStorageBuffer>()->GetVulkanDescriptorInfo(frameIndex);

							writeDescriptor.pBufferInfo = bufferInfos[bufferInfoIndex].data();
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
					ATN_CORE_INFO_TAG("Renderer", "DescriptorSetManager '{}' - Updating descriptors in set {} (frameIndex {})", m_Info.Name, set, frameIndex);
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
				WriteDescriptorSet& storedWriteDescriptor = m_WriteDescriptorSetTable[frameIndex].at(set).at(binding);
				ResourceState& resourceState = storedWriteDescriptor.ResourcesState[0];

				switch (resource.Type)
				{
				case ShaderResourceType::Texture2D:
				{
					for (uint32 i = 0; i < resource.Storage.size(); ++i)
					{
						const auto& imageInfo = resource.Storage[i].As<VulkanTexture2D>()->GetVulkanDescriptorInfo();
						if (IsResourceStateChanged(storedWriteDescriptor.ResourcesState[i], imageInfo))
						{
							m_InvalidatedResources[set][binding] = resource;
							break;
						}
					}

					break;
				}
				case ShaderResourceType::TextureCube:
				{
					const auto& imageInfo = resource.Storage[0].As<VulkanTextureCube>()->GetVulkanDescriptorInfo();
					if (IsResourceStateChanged(resourceState, imageInfo))
						m_InvalidatedResources[set][binding] = resource;

					break;
				}
				case ShaderResourceType::UniformBuffer:
				{
					const auto& bufferInfo = resource.Storage[0].As<VulkanUniformBuffer>()->GetVulkanDescriptorInfo(frameIndex);
					if (IsResourceStateChanged(resourceState, bufferInfo))
						m_InvalidatedResources[set][binding] = resource;

					break;
				}
				case ShaderResourceType::StorageBuffer:
				{
					const auto& bufferInfo = resource.Storage[0].As<VulkanStorageBuffer>()->GetVulkanDescriptorInfo(frameIndex);
					if (IsResourceStateChanged(resourceState, bufferInfo))
						m_InvalidatedResources[set][binding] = resource;

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
				case ShaderResourceType::Texture2D:
				{
					uint32 imageInfoIndex = imageInfos.size();
					imageInfos.emplace_back(resource.Storage.size());

					for (uint32 i = 0; i < resource.Storage.size(); ++i)
					{
						ResourceState& resourceState = storedWriteDescriptor.ResourcesState[i];
						VkDescriptorImageInfo& imageInfo = imageInfos[imageInfoIndex][i];
						imageInfo = resource.Storage[i].As<VulkanTexture2D>()->GetVulkanDescriptorInfo();

						writeDescriptor.pImageInfo = imageInfos[imageInfoIndex].data();
						resourceState.ResourceHandle = imageInfo.imageView;
						resourceState.ImageLayout = imageInfo.imageLayout;
						resourceState.Sampler = imageInfo.sampler;
					}
					break;
				}
				case ShaderResourceType::TextureCube:
				{
					uint32 imageInfoIndex = imageInfos.size();
					imageInfos.emplace_back(resource.Storage.size());

					for (uint32 i = 0; i < resource.Storage.size(); ++i)
					{
						ResourceState& resourceState = storedWriteDescriptor.ResourcesState[i];
						VkDescriptorImageInfo& imageInfo = imageInfos[imageInfoIndex][i];
						imageInfo = resource.Storage[i].As<VulkanTextureCube>()->GetVulkanDescriptorInfo();

						writeDescriptor.pImageInfo = imageInfos[imageInfoIndex].data();
						resourceState.ResourceHandle = imageInfo.imageView;
						resourceState.ImageLayout = imageInfo.imageLayout;
						resourceState.Sampler = imageInfo.sampler;
					}
					break;
				}
				case ShaderResourceType::UniformBuffer:
				{
					uint32 bufferInfoIndex = bufferInfos.size();
					bufferInfos.emplace_back(resource.Storage.size());

					for (uint32 i = 0; i < resource.Storage.size(); ++i)
					{
						ResourceState& resourceState = storedWriteDescriptor.ResourcesState[i];
						VkDescriptorBufferInfo& bufferInfo = bufferInfos[bufferInfoIndex][i];
						bufferInfo = resource.Storage[i].As<VulkanUniformBuffer>()->GetVulkanDescriptorInfo(frameIndex);

						writeDescriptor.pBufferInfo = bufferInfos[bufferInfoIndex].data();
						resourceState.ResourceHandle = bufferInfo.buffer;
					}
					break;
				}
				case ShaderResourceType::StorageBuffer:
				{
					uint32 bufferInfoIndex = bufferInfos.size();
					bufferInfos.emplace_back(resource.Storage.size());

					for (uint32 i = 0; i < resource.Storage.size(); ++i)
					{
						ResourceState& resourceState = storedWriteDescriptor.ResourcesState[i];
						VkDescriptorBufferInfo& bufferInfo = bufferInfos[bufferInfoIndex][i];
						bufferInfo = resource.Storage[i].As<VulkanStorageBuffer>()->GetVulkanDescriptorInfo(frameIndex);

						writeDescriptor.pBufferInfo = bufferInfos[bufferInfoIndex].data();
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

	bool DescriptorSetManager::IsResourceStateChanged(const ResourceState& oldState, const VkDescriptorImageInfo& imageInfo)
	{
		return !(oldState.ResourceHandle == imageInfo.imageView && oldState.ImageLayout == imageInfo.imageLayout && oldState.Sampler == imageInfo.sampler);
	}

	bool DescriptorSetManager::IsResourceStateChanged(const ResourceState& oldState, const VkDescriptorBufferInfo& bufferInfo)
	{
		return oldState.ResourceHandle != bufferInfo.buffer;
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
}
