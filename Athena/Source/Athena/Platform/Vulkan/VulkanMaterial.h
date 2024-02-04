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

		virtual void Set(const String& name, const Ref<ShaderResource>& resource) override;

		virtual Ref<Texture2D> GetTexture(const String& name) override;

		virtual void Bind(const Ref<RenderCommandBuffer>& commandBuffer) override;
		virtual void RT_UpdateForRendering(const Ref<RenderCommandBuffer>& commandBuffer) override;

		virtual void SetInternal(const String& name, ShaderDataType dataType, const void* data) override;
		virtual bool GetInternal(const String& name, ShaderDataType dataType, void** data) override;

	private:
		bool Exists(const String& name, ShaderDataType dataType);

	private:
		Ref<DescriptorSetManager> m_DescriptorSetManager;
		byte m_PushConstantBuffer[128];
	};
}
