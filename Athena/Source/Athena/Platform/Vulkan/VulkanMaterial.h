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

		virtual void Set(const String& name, const Ref<RenderResource>& resource, uint32 arrayIndex) override;
		virtual void Set(const String& name, const Ref<Texture>& resource, uint32 arrayIndex, uint32 mip) override;
		virtual Ref<RenderResource> GetResourceInternal(const String& name) override;

		virtual void Bind(const Ref<RenderCommandBuffer>& commandBuffer) override;

	private:
		VkPipelineBindPoint m_PipelineBindPoint;
		DescriptorSetManager m_DescriptorSetManager;
	};
}
