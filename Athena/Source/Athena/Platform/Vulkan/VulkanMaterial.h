#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Material.h"
#include "Athena/Platform/Vulkan/DescriptorSetManager.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	class VulkanMaterial : public Material
	{
	public:
		VulkanMaterial(const Ref<Shader>& shader, const String& name);
		~VulkanMaterial();

		virtual void Set(std::string_view name, const Ref<ShaderResource>& resource) override;
		virtual void Set(std::string_view name, const Matrix4& mat4) override;

		virtual void Bind() override;
		virtual void RT_UpdateForRendering() override;

	private:
		Ref<DescriptorSetManager> m_DescriptorSetManager;
		byte m_PushConstantBuffer[128];
	};
}
