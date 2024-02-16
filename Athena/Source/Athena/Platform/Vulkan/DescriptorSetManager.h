#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/ShaderResource.h"
#include "Athena/Renderer/Shader.h"
#include "Athena/Renderer/Texture.h"

#include <vulkan/vulkan.h>
#include <unordered_map>


namespace Athena
{
	struct RenderResourceStorage
	{
		std::vector<Ref<RenderResource>> Storage;
		RenderResourceType Type;
		uint32 MipLevel = 0;	// TODO: different mip level for each texture in array
	};

	// map of set -> binding -> render resource array
	using RenderResourcesTable = std::unordered_map<uint32, std::unordered_map<uint32, RenderResourceStorage>>;

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

		void Set(const String& name, const Ref<RenderResource>& resource, uint32 arrayIndex = 0);
		void Set(const String& name, const Ref<Texture>& resource, uint32 arrayIndex, uint32 mip);
		Ref<RenderResource> Get(const String& name, uint32 arrayIndex = 0);

		bool Validate() const;
		void Bake();
		void RT_InvalidateAndUpdate();
		void RT_BindDescriptorSets(VkCommandBuffer vkcommandBuffer, VkPipelineBindPoint bindPoint);

		bool IsInvalidated(uint32 set, uint32 binding);

	private:
		RenderResourceStorage* GetResourceStorage(const String& name, uint32 arrayIndex);
		bool IsCompatible(RenderResourceType renderType, ShaderResourceType shaderType) const;
		bool IsValidSet(uint32 set) const;

		bool IsResourceStateChanged(const ResourceState& oldState, const VkDescriptorImageInfo& imageInfo);
		bool IsResourceStateChanged(const ResourceState& oldState, const VkDescriptorBufferInfo& bufferInfo);

	private:
		DescriptorSetManagerCreateInfo m_Info;
		const std::unordered_map<String, ShaderResourceDescription>* m_ResourcesDescriptionTable;
		RenderResourcesTable m_Resources;
		RenderResourcesTable m_InvalidatedResources;
		std::vector<std::vector<VkDescriptorSet>> m_DescriptorSets;
		std::vector<std::unordered_map<uint32, std::unordered_map<uint32, WriteDescriptorSet>>> m_WriteDescriptorSetTable;
		VkDescriptorPool m_DescriptorPool;
	};
}
