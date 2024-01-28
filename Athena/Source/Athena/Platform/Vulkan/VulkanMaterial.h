#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Material.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	class VulkanMaterial : public Material
	{
	public:
		VulkanMaterial(const Ref<Shader>& shader);
		~VulkanMaterial();

		virtual void Set(std::string_view name, const Ref<ShaderResource>& resource) override;
		virtual void Set(std::string_view name, const Matrix4& mat4) override;

		virtual void RT_Bind() override;
		virtual void RT_UpdateForRendering() override;

	private:
		struct ResourceDescription
		{
			Ref<ShaderResource> Resource;
			uint32 Binding;
		};

	private:
		std::vector<std::vector<VkDescriptorSet>> m_DescriptorSets;
		std::unordered_map<uint32, std::unordered_map<std::string_view, ResourceDescription>> m_ResourcesTable;
		byte m_PushConstantBuffer[128];
	};
}
