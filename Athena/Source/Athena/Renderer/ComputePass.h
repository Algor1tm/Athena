#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Image.h"
#include "Athena/Renderer/RenderCommandBuffer.h"


namespace Athena
{
	struct ComputePassCreateInfo
	{
		String Name;
		std::vector<Ref<Image>> Outputs;
	};

	class ATHENA_API ComputePass: public RefCounted
	{
	public:
		static Ref<ComputePass> Create(const ComputePassCreateInfo& info);
		virtual ~ComputePass() = default;

		virtual void Begin(const Ref<RenderCommandBuffer>& commandBuffer) = 0;
		virtual void End(const Ref<RenderCommandBuffer>& commandBuffer) = 0;

		Ref<Image> GetOutput(uint32 index) const { return m_Info.Outputs[index]; }
		const ComputePassCreateInfo& GetInfo() const { return m_Info; }

	protected:
		ComputePassCreateInfo m_Info;
	};
}
