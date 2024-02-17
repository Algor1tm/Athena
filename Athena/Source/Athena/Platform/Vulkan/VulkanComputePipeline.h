#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/ComputePipeline.h"
#include "Athena/Platform/Vulkan/DescriptorSetManager.h"


namespace Athena
{
	class ATHENA_API VulkanComputePipeline : public ComputePipeline
	{
	public:
		VulkanComputePipeline(const ComputePipelineCreateInfo& info);
		~VulkanComputePipeline();

		virtual void Bind(const Ref<RenderCommandBuffer>& commandBuffer) override;

		virtual void SetInput(const String& name, const Ref<RenderResource>& resource) override;
		virtual void Bake() override;

	private:
		DescriptorSetManager m_DescriptorSetManager;
		VkPipeline m_VulkanPipeline;
	};
}