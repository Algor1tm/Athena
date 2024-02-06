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

	struct WriteDescriptorSet
	{
		void* ResourceHandle;
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

		void Set(const String& name, Ref<ShaderResource> resource);

		template <typename T>
		T Get(const String& name);

		bool Validate() const;
		void Bake();
		void RT_InvalidateAndUpdate();
		void RT_BindDescriptorSets(VkCommandBuffer vkcommandBuffer, VkPipelineBindPoint bindPoint);

		bool IsInvalidated(uint32 set, uint32 binding);

	private:
		DescriptorSetManagerCreateInfo m_Info;
		std::unordered_map<String, ShaderResourceDescription> m_ResourcesDescriptionTable;
		ShaderResourcesTable m_Resources;
		ShaderResourcesTable m_InvalidatedResources;
		std::vector<std::vector<VkDescriptorSet>> m_DescriptorSets;
		std::vector<std::unordered_map<uint32, std::unordered_map<uint32, WriteDescriptorSet>>> m_WriteDescriptorSetTable;
		VkDescriptorPool m_DescriptorPool;
	};


	// TODO: need to check types before upcasting
	template <typename T>
	T DescriptorSetManager::Get(const String& name)
	{
		if (m_ResourcesDescriptionTable.contains(name))
		{
			const ShaderResourceDescription& desc = m_ResourcesDescriptionTable.at(name);

			if (m_Resources.contains(desc.Set) && m_Resources.at(desc.Set).contains(desc.Binding))
			{
				return m_Resources.at(desc.Set).at(desc.Binding);
			}

			ATN_CORE_ERROR_TAG("Renderer", "DescriptorSetManager '{}' - Failed to get shader resource with name '{}' (resource is not present)", m_Info.Name, name);
		}
		else
		{
			ATN_CORE_ERROR_TAG("Renderer", "DescriptorSetManager '{}' - Failed to get shader resource with name '{}' (invalid name)", m_Info.Name, name);
		}

		return nullptr;
	}
}
