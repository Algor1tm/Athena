#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Pipeline.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	class VulkanPipeline : public Pipeline
	{
	public:
		VulkanPipeline(const PipelineCreateInfo& info);
		~VulkanPipeline();

		virtual void Bind() override;
		virtual void SetViewport(uint32 width, uint32 height) override;

	private:
		void CleanUp();
		void CreatePipeline(uint32 width, uint32 height);

	private:
		VkPipeline m_VulkanPipeline;
	};
}
