#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Framebuffer.h"


namespace Athena
{
	struct RenderPassCreateInfo
	{
		Ref<Framebuffer> OutputTarget;
		bool LoadOpClear;
	};

	class ATHENA_API RenderPass: public RefCounted
	{
	public:
		static Ref<RenderPass> Create(const RenderPassCreateInfo& info);
		virtual ~RenderPass() = default;

		const RenderPassCreateInfo& GetInfo() const { return m_Info; }

	protected:
		RenderPassCreateInfo m_Info;
	};
}
