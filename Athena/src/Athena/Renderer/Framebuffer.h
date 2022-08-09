#pragma once

#include "Athena/Core/Core.h"


namespace Athena
{
	struct FramebufferDesc
	{
		uint32 Width, Height;
		uint32 Samples = 1;

		bool SwapChainTarget = false;
	};

	class ATHENA_API Framebuffer
	{
	public:
		virtual ~Framebuffer() = default;

		virtual void Resize(uint32 width, uint32 height) = 0;

		virtual const FramebufferDesc& GetDescription() const = 0;
		virtual uint32 GetColorAttachmentRendererID() const = 0;

		virtual void Bind() const = 0;
		virtual void UnBind() const = 0;

		static Ref<Framebuffer> Create(const FramebufferDesc& desc);
	};
}
	