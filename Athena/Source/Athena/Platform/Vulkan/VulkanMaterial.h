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

		virtual void Set(std::string_view name, const Matrix4& value) override;
		virtual void Set(std::string_view name, const Vector4& value) override;
		virtual void Set(std::string_view name, float value) override;
		virtual void Set(std::string_view name, uint32 value) override;

		virtual void Bind() override;
		virtual void RT_UpdateForRendering() override;

	private:
		void RT_SetPushConstantData(std::string_view name, ShaderDataType dataType, const void* data);

	private:
		Ref<DescriptorSetManager> m_DescriptorSetManager;
		byte m_PushConstantBuffer[128];
	};
}
