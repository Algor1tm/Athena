#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Renderer/Framebuffer.h"


typedef unsigned int GLenum;

namespace Athena
{
	class ATHENA_API GLFramebuffer: public Framebuffer
	{
	public:
		GLFramebuffer(const FramebufferDescription& desc);
		virtual ~GLFramebuffer();

		void Recreate();

		virtual void Resize(uint32 width, uint32 height) override;

		virtual void Bind() override;
		virtual void UnBind() override;

		virtual void BindColorAttachment(uint32 index, uint32 slot = 0) const override;
		virtual void BindDepthAttachment(uint32 slot = 0) const override;

		virtual const FramebufferDescription& GetDescription() const override { return m_Description; }

		virtual void* GetColorAttachmentRendererID(uint32 index = 0) const override;
		virtual void* GetDepthAttachmentRendererID() const override;

		virtual int ReadPixel(uint32 attachmentIndex, int x, int y) override;
		virtual void ClearAttachment(uint32 attachmentIndex, int value) override;

		virtual void ResolveMutlisampling() override;
		virtual void BlitToScreen() const override;

	private:
		void DeleteAttachments();
		void AttachColorTexture(uint32 id, uint32 samples, GLenum internalFormat, GLenum format, GLenum dataType, uint32 width, uint32 height, uint32 depth, uint32 index);
		void AttachDepthTexture(uint32 id, uint32 samples, GLenum format, GLenum attachmentType, uint32 width, uint32 height, uint32 depth);

		void CreateFramebufferObject(uint32* rendererID, bool resolved);

		bool IsMultisample() const { return m_Description.Samples > 1; }

	private:
		uint32 m_FramebufferID = 0;		
		uint32 m_ResolvedFramebufferID = 0;		// if not multisample - invalid
		FramebufferDescription m_Description;

		std::vector<FramebufferTextureDescription> m_ColorAttachmentDescriptions;
		std::vector<uint32> m_ColorAttachments;
		std::vector<uint32> m_ColorAttachmentsResolved; // if not multisample - invalid

		FramebufferTextureDescription m_DepthAttachmentDescription = TextureFormat::NONE;
		uint32 m_DepthAttachment;
	};
}
