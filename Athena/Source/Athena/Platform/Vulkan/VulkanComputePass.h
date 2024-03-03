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

		virtual void Bake() override;

	private:
		struct BarrierInfo
		{
			VkAccessFlags SrcAccess;
			VkPipelineStageFlags SrcStageFlags;
		};

	private:
		std::vector<BarrierInfo> m_Barriers;
	};
}
