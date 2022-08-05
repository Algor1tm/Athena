#pragma once

#include "Athena/Renderer/Framebuffer.h"


namespace Athena
{
	class ATHENA_API OpenGLFramebuffer: public Framebuffer
	{
	public:
		OpenGLFramebuffer(const FramebufferDesc& desc);
		virtual ~OpenGLFramebuffer();

		void Recreate();

		virtual const FramebufferDesc& GetDescription() const override { return m_Description; }
		virtual uint32_t GetColorAttachmentRendererID() const override { return m_ColorAttachment; }

		virtual void Bind() const override;
		virtual void UnBind() const override;

	private:
		uint32_t m_RendererID;
		uint32_t m_ColorAttachment, m_DepthAttachment;
		FramebufferDesc m_Description;
	};
}
