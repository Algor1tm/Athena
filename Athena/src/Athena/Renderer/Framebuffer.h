#pragma once

#include "Athena/Core/Core.h"

#include <vector>


namespace Athena
{
	enum class FramebufferTextureFormat
	{
		None = 0,

		// Color
		RGBA8,

		//Depth/Stencil
		DEPTH24STENCIL8,

		//Defaults
		Depth = DEPTH24STENCIL8
	};

	struct FramebufferTextureDESC
	{
		FramebufferTextureDESC(FramebufferTextureFormat format = FramebufferTextureFormat::None)
			: TextureFormat(format) {}

		FramebufferTextureFormat TextureFormat;
	};
	 
	struct FramebufferAttachmentDESC
	{
		FramebufferAttachmentDESC() = default;
		FramebufferAttachmentDESC(const std::initializer_list<FramebufferTextureDESC>& attachments)
			: Attachments(attachments) {}

		std::vector<FramebufferTextureDESC> Attachments;
	};

	struct FramebufferDESC
	{
		uint32 Width, Height;
		FramebufferAttachmentDESC Attachments;
		uint32 Samples = 1;

		bool SwapChainTarget = false;
	};

	class ATHENA_API Framebuffer
	{
	public:
		virtual ~Framebuffer() = default;

		virtual void Resize(uint32 width, uint32 height) = 0;

		virtual const FramebufferDESC& GetDescription() const = 0;
		virtual uint32 GetColorAttachmentRendererID(SIZE_T index = 0) const = 0;

		virtual void Bind() const = 0;
		virtual void UnBind() const = 0;

		static Ref<Framebuffer> Create(const FramebufferDESC& desc);
	};
}
	