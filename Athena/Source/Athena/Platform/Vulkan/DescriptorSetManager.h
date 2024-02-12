#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/ShaderResource.h"
#include "Athena/Renderer/Shader.h"

#include <vulkan/vulkan.h>
#include <unordered_map>


namespace Athena
{
	// map of set -> binding -> shader resource
	using ShaderResourcesTable = std::unordered_map<uint32, std::unordered_map<uint32, Ref<ShaderResource>>>;

	struct ShaderResourceDescription
	{
		ShaderResourceType Type;
		uint32 Binding;
		uint32 Set;
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
		ResourceState ResourceState;
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

		void Set(const String& name, const Ref<ShaderResource>& resource);
		Ref<ShaderResource> Get(const String& name);

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
