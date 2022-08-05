#pragma once

#include "Athena/Core/Core.h"


namespace Athena
{
	struct FramebufferDesc
	{
		uint32_t Width, Height;
		uint32_t Samples = 1;

		bool SwapChainTarget = false;
	};

	class ATHENA_API Framebuffer
	{
	public:
		virtual ~Framebuffer() = default;

		virtual const FramebufferDesc& GetDescription() const = 0;
		virtual uint32_t GetColorAttachmentRendererID() const = 0;

		virtual void Bind() const = 0;
		virtual void UnBind() const = 0;

		static Ref<Framebuffer> Create(const FramebufferDesc& desc);
	};
}
	