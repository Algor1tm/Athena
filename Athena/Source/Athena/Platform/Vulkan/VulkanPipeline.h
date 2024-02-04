#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Pipeline.h"
#include "Athena/Platform/Vulkan/DescriptorSetManager.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	class VulkanPipeline : public Pipeline
	{
	public:
		VulkanPipeline(const PipelineCreateInfo& info);
		~VulkanPipeline();

		virtual void Bind(const Ref<RenderCommandBuffer>& commandBuffer) override;
		virtual void SetViewport(uint32 width, uint32 height) override;

		virtual void SetInput(const String& name, Ref<ShaderResource> resource) override;
		virtual void Bake() override;

	private:
		void CleanUp();
		void CreatePipeline(uint32 width, uint32 height);

	private:
		Ref<DescriptorSetManager> m_DescriptorSetManager;
		VkPipeline m_VulkanPipeline;
	};
}
