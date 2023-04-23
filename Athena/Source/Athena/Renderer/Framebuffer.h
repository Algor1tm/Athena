#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Renderer/Color.h"
#include "Athena/Renderer/Texture.h"

#include <vector>


namespace Athena
{
	struct FramebufferAttachmentDescription
	{
		FramebufferAttachmentDescription(TextureFormat format = TextureFormat::NONE, bool generateMipMap = false)
			: Format(format), GenerateMipMap(generateMipMap) {}

		TextureFormat Format;
		bool GenerateMipMap;
	};
	
	struct FramebufferDescription
	{
		uint32 Width = 1;
		uint32 Height = 1;
		uint32 Layers = 1;

		std::vector<FramebufferAttachmentDescription> Attachments;
		uint32 Samples = 1;
	};

	class ATHENA_API Framebuffer
	{
	public:
		virtual ~Framebuffer() = default;
		static Ref<Framebuffer> Create(const FramebufferDescription& desc);

		virtual void Resize(uint32 width, uint32 height) = 0;

		virtual void Bind() = 0;
		virtual void UnBind() = 0;

		virtual void BindColorAttachmentAsImage(uint32 index, uint32 slot = 0, uint32 mipLevel = 0) const = 0;
		virtual void BindColorAttachment(uint32 index, uint32 slot = 0) const = 0;
		virtual void BindDepthAttachment(uint32 slot = 0) const = 0;

		virtual const FramebufferDescription& GetDescription() const = 0;

		virtual void* GetColorAttachmentRendererID(uint32 index = 0) const = 0;
		virtual void* GetDepthAttachmentRendererID() const = 0;

		virtual int ReadPixel(uint32 attachmentIndex, int x, int y) = 0;
		virtual void ClearAttachment(uint32 attachmentIndex, int value) = 0;

		virtual void ResolveMutlisampling() = 0;
		virtual void BlitToScreen() const = 0;
	};
}
