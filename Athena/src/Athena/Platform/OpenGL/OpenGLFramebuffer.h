#pragma once

#include "Athena/Renderer/Framebuffer.h"


namespace Athena
{
	class ATHENA_API OpenGLFramebuffer: public Framebuffer
	{
	public:
		OpenGLFramebuffer(const FramebufferDesc& desc);
		~OpenGLFramebuffer();

		void Recreate();

		void Resize(uint32_t width, uint32_t height) override;

		virtual const FramebufferDesc& GetDescription() const override { return m_Description; }
		virtual uint32_t GetColorAttachmentRendererID() const override { return m_ColorAttachment; }

		virtual void Bind() const override;
		virtual void UnBind() const override;

	private:
		RendererID m_RendererID = 0;
		RendererID m_ColorAttachment = 0, m_DepthAttachment = 0;
		FramebufferDesc m_Description;
	};
}
