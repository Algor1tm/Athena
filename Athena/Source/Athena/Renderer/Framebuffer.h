#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Texture.h"


namespace Athena
{
	struct FramebufferAttachmentInfo
	{
		FramebufferAttachmentInfo(ImageFormat format)
			: Format(format){}

		String Name;
		ImageFormat Format;
		LinearColor ClearColor = Vector4(0.f);
		float DepthClearColor = 0.f;
		uint32 StencilClearColor = 0.f;
	};

	struct FramebufferCreateInfo
	{
		String Name;
		std::vector<FramebufferAttachmentInfo> Attachments;
		uint32 Width = 1;
		uint32 Height = 1;
	};

	class ATHENA_API Framebuffer : public RefCounted
	{
	public:
		static Ref<Framebuffer> Create(const FramebufferCreateInfo& info);
		virtual ~Framebuffer() = default;

		virtual void Resize(uint32 width, uint32 height) = 0;

		virtual const Ref<Texture2D>& GetColorAttachment(uint32 index = 0) const = 0;
		virtual const Ref<Texture2D>& GetDepthAttachment() const = 0;

		virtual uint32 GetColorAttachmentCount() const = 0;
		virtual bool HasDepthAttachment() const = 0;

		const FramebufferCreateInfo& GetInfo() const { return m_Info; }

	protected:
		FramebufferCreateInfo m_Info;
	};
}
