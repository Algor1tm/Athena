#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Renderer/Color.h"
#include "Athena/Renderer/Texture.h"

#include <vector>


namespace Athena
{
	struct FramebufferTextureDescription
	{
		FramebufferTextureDescription(TextureFormat format = TextureFormat::NONE)
			: Format(format) {}

		TextureFormat Format;
	};
	
	struct FramebufferAttachmentDescription
	{
		FramebufferAttachmentDescription() = default;
		FramebufferAttachmentDescription(const std::initializer_list<FramebufferTextureDescription>& attachments)
			: Attachments(attachments) {}

		std::vector<FramebufferTextureDescription> Attachments;
	};

	struct FramebufferDescription
	{
		uint32 Width = 0, Height = 0;
		FramebufferAttachmentDescription Attachments;
		uint32 Samples = 1;
	};

	class ATHENA_API Framebuffer
	{
	public:
		virtual ~Framebuffer() = default;
		static Ref<Framebuffer> Create(const FramebufferDescription& desc);

		virtual void Resize(uint32 width, uint32 height) = 0;

		virtual const FramebufferDescription& GetDescription() const = 0;
		virtual void* GetColorAttachmentRendererID(uint32 index = 0) const = 0;

		virtual int ReadPixel(uint32 attachmentIndex, int x, int y) = 0;
		virtual void ClearAttachment(uint32 attachmentIndex, int value) = 0;

		virtual void ClearColorAndDepth(const LinearColor& color) = 0;

		virtual void ResolveMutlisampling() = 0;

		virtual void ReplaceAttachment(uint32 attachmentIndex, TextureTarget textureTarget, void* rendererID, uint32 level = 0) = 0;
	};
}
