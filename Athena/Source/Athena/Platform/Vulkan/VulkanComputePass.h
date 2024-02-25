#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/ComputePass.h"
#include "Athena/Renderer/Material.h"
#include "Athena/Platform/Vulkan/DescriptorSetManager.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	class VulkanComputePass : public ComputePass
	{
	public:
		VulkanComputePass(const ComputePassCreateInfo& info);
		~VulkanComputePass();

		virtual void Begin(const Ref<RenderCommandBuffer>& commandBuffer) override;
		virtual void End(const Ref<RenderCommandBuffer>& commandBuffer) override;

		virtual void SetInput(const String& name, const Ref<RenderResource>& resource) override;
		virtual void Bake() override;

		void RT_SetPushConstants(VkCommandBuffer commandBuffer, const Ref<Material>& material);
		Vector3u GetWorkGroupSize() const { return m_WorkGroupSize; }

	private:
		void CleanUp();
		void RecreatePipeline();

	private:
		DescriptorSetManager m_DescriptorSetManager;
		VkPipeline m_VulkanPipeline;

		uint64 m_Hash;
		Vector3u m_WorkGroupSize;
		VkPipelineLayout m_PipelineLayout;
		VkShaderStageFlags m_PushConstantStageFlags;
		uint32 m_PushConstantSize;
	};
}
