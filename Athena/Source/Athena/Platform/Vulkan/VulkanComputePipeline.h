#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/ComputePipeline.h"
#include "Athena/Renderer/Material.h"
#include "Athena/Platform/Vulkan/DescriptorSetManager.h"


namespace Athena
{
	class ATHENA_API VulkanComputePipeline : public ComputePipeline
	{
	public:
		VulkanComputePipeline(const Ref<Shader>& shader);
		~VulkanComputePipeline();

		virtual bool Bind(const Ref<RenderCommandBuffer>& commandBuffer) override;

		virtual void SetInput(const String& name, const Ref<RenderResource>& resource) override;
		virtual void Bake() override;

		Vector3u GetWorkGroupSize() const;

		void RT_SetPushConstants(VkCommandBuffer commandBuffer, const Ref<Material>& material);

	private:
		void CleanUp();
		void RecreatePipeline();

	private:
		DescriptorSetManager m_DescriptorSetManager;
		VkPipeline m_VulkanPipeline;
		Vector3u m_WorkGroupSize;
		uint64 m_Hash;

		VkPipelineLayout m_PipelineLayout;
		VkShaderStageFlags m_PushConstantStageFlags;
		uint32 m_PushConstantSize;
	};
}