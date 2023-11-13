#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Framebuffer.h"


namespace Athena
{
	struct RenderPassCreateInfo
	{
		Ref<Framebuffer> Output;
		bool LoadOpClear;
	};

	class ATHENA_API RenderPass: public RefCounted
	{
	public:
		static Ref<RenderPass> Create(const RenderPassCreateInfo& info);
		virtual ~RenderPass() = default;

		virtual void Begin() = 0;
		virtual void End() = 0;

		const RenderPassCreateInfo& GetInfo() const { return m_Info; }

	protected:
		RenderPassCreateInfo m_Info;
	};
}
