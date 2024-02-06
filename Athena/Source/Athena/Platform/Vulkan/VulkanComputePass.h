#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/ComputePass.h"


namespace Athena
{
	class VulkanComputePass : public ComputePass
	{
	public:
		VulkanComputePass(const ComputePassCreateInfo& info);
		~VulkanComputePass();

		virtual void Begin(const Ref<RenderCommandBuffer>& commandBuffer) override;
		virtual void End(const Ref<RenderCommandBuffer>& commandBuffer) override;
	};
}
