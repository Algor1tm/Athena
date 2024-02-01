#include "DescriptorSetManager.h"
#include "Athena/Platform/Vulkan/VulkanTexture2D.h"
#include "Athena/Platform/Vulkan/VulkanUniformBuffer.h"
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
			case ShaderResourceType::UniformBuffer: return "UniformBuffer";
			case ShaderResourceType::Texture2D: return "Texture2D";
			}

			ATN_CORE_ASSERT(false);
			return "";
		}
	}

	DescriptorSetManager::DescriptorSetManager(const DescriptorSetManagerCreateInfo& info)
	{
		m_Info = info;

		Renderer::Submit([this]()
		{
			const ShaderReflectionData& reflectionData = m_Info.Shader->GetReflectionData();

			for (const auto& [name, ubo] : reflectionData.UniformBuffers)
			{
				ShaderResourceDescription resourceDesc;
				resourceDesc.Type = ShaderResourceType::UniformBuffer;
				resourceDesc.Binding = ubo.Binding;
				resourceDesc.Set = ubo.Set;

				if (resourceDesc.Set >= m_Info.FirstSet && resourceDesc.Set <= m_Info.LastSet)
					m_ResourcesDescriptionTable[name] = resourceDesc;
			}
			for (const auto& [name, texture] : reflectionData.Textures2D)
			{
				ShaderResourceDescription resourceDesc;
				resourceDesc.Type = ShaderResourceType::Texture2D;
				resourceDesc.Binding = texture.Binding;
				resourceDesc.Set = texture.Set;

				if (resourceDesc.Set >= m_Info.FirstSet && resourceDesc.Set <= m_Info.LastSet)
					m_ResourcesDescriptionTable[name] = resourceDesc;
			}

			// Insert default textures for set 0
			for (const auto& [name, resDesc] : m_ResourcesDescriptionTable)
			{
				if (resDesc.Type == ShaderResourceType::Texture2D && resDesc.Set == 0)
				{
					m_Resources[resDesc.Set][resDesc.Binding] = Renderer::GetWhiteTexture();
				}
			}
		});
	}

	DescriptorSetManager::~DescriptorSetManager()
	{
		Renderer::SubmitResourceFree([pool = m_DescriptorPool]()
		{
			if(pool != VK_NULL_HANDLE)
				vkDestroyDescriptorPool(VulkanContext::GetLogicalDevice(), pool, nullptr);
		});
	}

	void DescriptorSetManager::Set(std::string_view name, Ref<ShaderResource> resource)
	{
		Renderer::Submit([this, name, resource]()
		{
			String nameStr(name);
			if (m_ResourcesDescriptionTable.contains(nameStr))
			{
				const ShaderResourceDescription& desc = m_ResourcesDescriptionTable.at(nameStr);
				m_Resources[desc.Set][desc.Binding] = resource;
			}
			else
			{
				ATN_CORE_ERROR_TAG("Renderer", "DescriptorSetManager '{}' - Failed to set shader resource with name '{}' (invalid name)", m_Info.Name, name);
			}
		});
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

			if (resource == nullptr)
			{
				ATN_CORE_ERROR_TAG("Renderer", "DescriptorSetManager '{}' - Resource '{}' is NULL (set {}, binding {})!", m_Info.Name, name, resDesc.Set, resDesc.Binding);
				return false;
			}

			if (resDesc.Type != resource->GetResourceType())
			{
				ATN_CORE_ERROR_TAG("Renderer", "DescriptorSetManager '{}' - Required resource '{}' is wrong type (expected - '{}', given - '{}')", m_Info.Name, name, Utils::ResourceTypeToString(resDesc.Type), Utils::ResourceTypeToString(resource->GetResourceType()));
				return false;
			}
		}

		return true;
	}

	void DescriptorSetManager::Bake()
	{
		Renderer::Submit([this]()
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
			for (const auto& [name, resDesc] : m_ResourcesDescriptionTable)
			{
				switch (resDesc.Type)
				{
				case ShaderResourceType::UniformBuffer:
				{
					poolSizesMap[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER] += 1;
					break;
				}
				case ShaderResourceType::Texture2D:
				{
					poolSizesMap[VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER] += 1;
					break;
				}
				}

				if (resDesc.Set > maxSet)
					maxSet = resDesc.Set;
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
			m_WriteDescriptorSetTable.resize(Renderer::GetFramesInFlight());

			for (uint32 frameIndex = 0; frameIndex < Renderer::GetFramesInFlight(); ++frameIndex)
			{
				if (setLayouts.size() > 0)
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
					for (const auto& [binding, resource] : setData)
					{
						WriteDescriptorSet& storedWriteDescriptor = m_WriteDescriptorSetTable[frameIndex][set][binding];
						VkWriteDescriptorSet& writeDescriptor = storedWriteDescriptor.VulkanWriteDescriptorSet;
						writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
						writeDescriptor.dstSet = m_DescriptorSets[frameIndex].at(set - m_Info.FirstSet);
						writeDescriptor.dstBinding = binding;
						writeDescriptor.dstArrayElement = 0;
						writeDescriptor.descriptorCount = 1;

						switch (resource->GetResourceType())
						{
						case ShaderResourceType::UniformBuffer:
						{
							Ref<VulkanUniformBuffer> ubo = resource.As<VulkanUniformBuffer>();

							writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
							writeDescriptor.pBufferInfo = &ubo->GetVulkanDescriptorInfo(frameIndex);
							storedWriteDescriptor.ResourceHandle = writeDescriptor.pBufferInfo->buffer;

							if (storedWriteDescriptor.ResourceHandle == nullptr)
								m_InvalidatedResources[set][binding] = resource;

							break;
						}
						case ShaderResourceType::Texture2D:
						{
							Ref<VulkanTexture2D> texture = resource.As<VulkanTexture2D>();

							writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
							writeDescriptor.pImageInfo = &texture->GetVulkanDescriptorInfo();
							storedWriteDescriptor.ResourceHandle = writeDescriptor.pImageInfo->imageView;

							if (storedWriteDescriptor.ResourceHandle == nullptr)
								m_InvalidatedResources[set][binding] = resource;

							break;
						}
						}
					}

					std::vector<VkWriteDescriptorSet> descriptorsToUpdate;
					for (const auto& [binding, writeDescriptor] : m_WriteDescriptorSetTable[frameIndex][set])
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
		});
	}

	void DescriptorSetManager::RT_InvalidateAndUpdate()
	{
		uint32 frameIndex = Renderer::GetCurrentFrameIndex();

		for (const auto& [set, setData] : m_Resources)
		{
			for (const auto& [binding, resource] : setData)
			{
				WriteDescriptorSet& storedWriteDescriptor = m_WriteDescriptorSetTable[frameIndex].at(set).at(binding);
				VkWriteDescriptorSet& writeDescriptor = storedWriteDescriptor.VulkanWriteDescriptorSet;

				switch (resource->GetResourceType())
				{
				case ShaderResourceType::UniformBuffer:
				{
					const auto& bufferInfo = resource.As<VulkanUniformBuffer>()->GetVulkanDescriptorInfo(frameIndex);
					if (&bufferInfo != writeDescriptor.pBufferInfo || storedWriteDescriptor.ResourceHandle != writeDescriptor.pBufferInfo->buffer)
						m_InvalidatedResources[set][binding] = resource;

					break;
				}
				case ShaderResourceType::Texture2D:
				{
					const auto& imageInfo = resource.As<VulkanTexture2D>()->GetVulkanDescriptorInfo();
					if (&imageInfo != writeDescriptor.pImageInfo || storedWriteDescriptor.ResourceHandle != writeDescriptor.pImageInfo->imageView)
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

			for (const auto& [binding, resource] : setData)
			{
				WriteDescriptorSet& storedWriteDescriptor = m_WriteDescriptorSetTable[frameIndex][set][binding];
				VkWriteDescriptorSet& writeDescriptor = storedWriteDescriptor.VulkanWriteDescriptorSet;

				switch (resource->GetResourceType())
				{
				case ShaderResourceType::UniformBuffer:
				{
					Ref<VulkanUniformBuffer> ubo = resource.As<VulkanUniformBuffer>();
					writeDescriptor.pBufferInfo = &ubo->GetVulkanDescriptorInfo(frameIndex);
					storedWriteDescriptor.ResourceHandle = writeDescriptor.pBufferInfo->buffer;
					break;
				}
				case ShaderResourceType::Texture2D:
				{
					Ref<VulkanTexture2D> texture = resource.As<VulkanTexture2D>();
					writeDescriptor.pImageInfo = &texture->GetVulkanDescriptorInfo();
					storedWriteDescriptor.ResourceHandle = writeDescriptor.pImageInfo->imageView;
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

	void DescriptorSetManager::RT_BindDescriptorSets()
	{
		const auto& descriptorSets = m_DescriptorSets[Renderer::GetCurrentFrameIndex()];

		if (descriptorSets.size() > 0)
		{
			vkCmdBindDescriptorSets(
				VulkanContext::GetActiveCommandBuffer(),
				VK_PIPELINE_BIND_POINT_GRAPHICS,
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
