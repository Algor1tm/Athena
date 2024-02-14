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

		virtual void SetResource(const String& name, const Ref<ShaderResource>& resource, uint32 arrayIndex) override;
		virtual Ref<ShaderResource> GetResource(const String& name) override;

		virtual void Bind(const Ref<RenderCommandBuffer>& commandBuffer) override;
		virtual void RT_SetPushConstant(const Ref<RenderCommandBuffer>& commandBuffer, const PushConstantRange& range) override;

	private:
		VkPipelineBindPoint m_PipelineBindPoint;
		Ref<DescriptorSetManager> m_DescriptorSetManager;
	};
}
