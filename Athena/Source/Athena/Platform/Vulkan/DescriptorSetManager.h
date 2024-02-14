#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/ShaderResource.h"
#include "Athena/Renderer/Shader.h"

#include <vulkan/vulkan.h>
#include <unordered_map>


namespace Athena
{
	struct ShaderResourceStorage
	{
		std::vector<Ref<ShaderResource>> Storage;
		ShaderResourceType Type;
	};

	// map of set -> binding -> shader resource array
	using ShaderResourcesTable = std::unordered_map<uint32, std::unordered_map<uint32, ShaderResourceStorage>>;

	struct ShaderResourceDescription
	{
		ShaderResourceType Type;
		uint32 Binding;
		uint32 Set;
		uint32 ArraySize;
	}; 

	struct ResourceState
	{
		// VkImageView or VkBuffer
		void* ResourceHandle;

		// texture specific
		VkImageLayout ImageLayout;
		VkSampler Sampler;
	};

	struct WriteDescriptorSet
	{
		std::vector<ResourceState> ResourcesState;
		VkWriteDescriptorSet VulkanWriteDescriptorSet;
	};

	struct DescriptorSetManagerCreateInfo
	{
		String Name;
		Ref<Shader> Shader;
		uint32 FirstSet;
		uint32 LastSet;
	};


	class ATHENA_API DescriptorSetManager : public RefCounted
	{
	public:
		DescriptorSetManager(const DescriptorSetManagerCreateInfo& info);
		~DescriptorSetManager();

		void Set(const String& name, const Ref<ShaderResource>& resource, uint32 arrayIndex = 0);
		Ref<ShaderResource> Get(const String& name, uint32 arrayIndex = 0);

		bool Validate() const;
		void Bake();
		void RT_InvalidateAndUpdate();
		void RT_BindDescriptorSets(VkCommandBuffer vkcommandBuffer, VkPipelineBindPoint bindPoint);

		bool IsInvalidated(uint32 set, uint32 binding);

	private:
		bool IsResourceStateChanged(const ResourceState& oldState, const VkDescriptorImageInfo& imageInfo);
		bool IsResourceStateChanged(const ResourceState& oldState, const VkDescriptorBufferInfo& bufferInfo);

	private:
		DescriptorSetManagerCreateInfo m_Info;
		std::unordered_map<String, ShaderResourceDescription> m_ResourcesDescriptionTable;
		ShaderResourcesTable m_Resources;
		ShaderResourcesTable m_InvalidatedResources;
		std::vector<std::vector<VkDescriptorSet>> m_DescriptorSets;
		std::vector<std::unordered_map<uint32, std::unordered_map<uint32, WriteDescriptorSet>>> m_WriteDescriptorSetTable;
		VkDescriptorPool m_DescriptorPool;
	};
}
