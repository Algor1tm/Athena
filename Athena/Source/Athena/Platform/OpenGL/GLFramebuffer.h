#pragma once

#include "Athena/Core/Log.h"
#include "Athena/Renderer/Framebuffer.h"

// TODO: Remove
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

		virtual const FramebufferDescription& GetDescription() const override { return m_Description; }
		virtual void* GetColorAttachmentRendererID(SIZE_T index = 0) const override;

		virtual int ReadPixel(SIZE_T attachmentIndex, int x, int y) override;
		virtual void ClearAttachment(SIZE_T attachmentIndex, int value) override;

		virtual void ClearColorAndDepth(const LinearColor& color) override;

		virtual void ResolveMutlisampling() override;

	private:
		void DeleteAttachments();
		void AttachColorTexture(uint32 id, uint32 samples, GLenum internalFormat, GLenum format, uint32 width, uint32 height, SIZE_T index);
		void AttachDepthTexture(uint32 id, uint32 samples, GLenum format, GLenum attachmentType, uint32 width, uint32 height);

		void CreateFramebufferObject(uint32* rendererID, bool resolved);

		bool IsMultisample() const { return m_Description.Samples > 1; }

	private:
		uint32 m_FramebufferID = 0;		
		uint32 m_ResolvedFramebufferID = 0;		// if not multisample - invalid
		FramebufferDescription m_Description;

		std::vector<FramebufferTextureDescription> m_ColorAttachmentDescriptions;
		std::vector<uint32> m_ColorAttachments;
		std::vector<uint32> m_ColorAttachmentsResolved; // if not multisample - invalid

		FramebufferTextureDescription m_DepthAttachmentDescription = FramebufferTextureFormat::NONE;
		uint32 m_DepthAttachment;
	};
}
