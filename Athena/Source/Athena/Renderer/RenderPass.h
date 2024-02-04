#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Framebuffer.h"
#include "Athena/Renderer/RenderCommandBuffer.h"


namespace Athena
{
	enum class RenderPassLoadOp
	{
		DONT_CARE = 0,
		CLEAR = 2,
		LOAD = 1
	};

	struct RenderPassCreateInfo
	{
		String Name;
		Ref<Framebuffer> Output;
		RenderPassLoadOp LoadOpClear;
	};

	class ATHENA_API RenderPass: public RefCounted
	{
	public:
		static Ref<RenderPass> Create(const RenderPassCreateInfo& info);
		virtual ~RenderPass() = default;

		virtual void Begin(const Ref<RenderCommandBuffer>& commandBuffer) = 0;
		virtual void End(const Ref<RenderCommandBuffer>& commandBuffer) = 0;

		Ref<Framebuffer> GetOutput() const { return m_Info.Output; }
		Ref<Texture2D> GetOutput(uint32 attachmentIndex) const { return m_Info.Output->GetColorAttachment(attachmentIndex); }
		Ref<Texture2D> GetDepthOutput() const { return m_Info.Output->GetDepthAttachment(); }

		const RenderPassCreateInfo& GetInfo() const { return m_Info; }

	protected:
		RenderPassCreateInfo m_Info;
	};
}
