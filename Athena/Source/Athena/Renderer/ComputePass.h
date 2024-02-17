#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Color.h"
#include "Athena/Renderer/Image.h"
#include "Athena/Renderer/RenderCommandBuffer.h"


namespace Athena
{
	struct ComputePassCreateInfo
	{
		String Name;
		LinearColor DebugColor = LinearColor(0.f);
	};

	class ATHENA_API ComputePass: public RefCounted
	{
	public:
		static Ref<ComputePass> Create(const ComputePassCreateInfo& info);
		virtual ~ComputePass() = default;

		virtual void Begin(const Ref<RenderCommandBuffer>& commandBuffer) = 0;
		virtual void End(const Ref<RenderCommandBuffer>& commandBuffer) = 0;

		void SetOutput(const Ref<Image>& image)
		{
			m_Outputs.push_back(image);
		}

		const ComputePassCreateInfo& GetInfo() const { return m_Info; }

	protected:
		ComputePassCreateInfo m_Info;
		std::vector<Ref<Image>> m_Outputs;
	};
}
