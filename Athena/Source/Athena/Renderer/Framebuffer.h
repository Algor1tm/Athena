#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Color.h"

#include <vector>


namespace Athena
{
	enum class FramebufferTextureFormat
	{
		NONE = 0,

		// Color
		RGBA8,
		RED_INTEGER,

		//Depth/Stencil
		DEPTH24STENCIL8,
	};
	
	struct FramebufferTextureDescription
	{
		FramebufferTextureDescription(FramebufferTextureFormat format = FramebufferTextureFormat::NONE, bool backBufferOutput = false)
			: TextureFormat(format), BackBufferOutput(backBufferOutput) {}

		FramebufferTextureFormat TextureFormat;
		bool BackBufferOutput;
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

		virtual void Resize(uint32 width, uint32 height) = 0;

		virtual const FramebufferDescription& GetDescription() const = 0;
		virtual void* GetColorAttachmentRendererID(SIZE_T index = 0) const = 0;

		virtual int ReadPixel(SIZE_T attachmentIndex, int x, int y) = 0;
		virtual void ClearAttachment(SIZE_T attachmentIndex, int value) = 0;

		virtual void ClearColorAndDepth(const LinearColor& color) = 0;

		static Ref<Framebuffer> Create(const FramebufferDescription& desc);
	};
}
