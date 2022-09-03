#pragma once

#include "Athena/Renderer/Framebuffer.h"

// TODO: Remove
typedef unsigned int GLenum;

namespace Athena
{
	class ATHENA_API OpenGLFramebuffer: public Framebuffer
	{
	public:
		OpenGLFramebuffer(const FramebufferDESC& desc);
		~OpenGLFramebuffer();

		void Recreate();

		void Resize(uint32 width, uint32 height) override;

		virtual const FramebufferDESC& GetDescription() const override { return m_Description; }
		virtual uint32 GetColorAttachmentRendererID(SIZE_T index = 0) const override { ATN_CORE_ASSERT(index < m_ColorAttachments.size(), "subscript out of range"); return m_ColorAttachments[index]; }

		virtual int ReadPixel(SIZE_T attachmentIndex, int x, int y) override;
		virtual void ClearAttachment(SIZE_T attachmentIndex, int value) override;

		virtual void Bind() const override;
		virtual void UnBind() const override;

	private:
		void DeleteAttachments();
		void AttachColorTexture(RendererID id, uint32 samples, GLenum internalFormat, GLenum format, uint32 width, uint32 height, SIZE_T index);
		void AttachDepthTexture(RendererID id, uint32 samples, GLenum format, GLenum attachmentType, uint32 width, uint32 height);

	private:
		RendererID m_RendererID = 0;
		FramebufferDESC m_Description;

		std::vector<FramebufferTextureDESC> m_ColorAttachmentDescriptions;
		std::vector<RendererID> m_ColorAttachments;

		FramebufferTextureDESC m_DepthAttachmentDescription = FramebufferTextureFormat::None;
		RendererID m_DepthAttachment;
	};
}
