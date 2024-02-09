#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/RenderCommandBuffer.h"
#include "Athena/Renderer/Texture.h"


namespace Athena
{
	enum class AttachmentLoadOp
	{
		DONT_CARE = 0,
		CLEAR = 2,
		LOAD = 1
	};

	struct AttachmentInfo
	{
		AttachmentInfo(ImageFormat format)
			: Format(format) {}

		String Name;
		ImageFormat Format = ImageFormat::NONE;
		AttachmentLoadOp LoadOp = AttachmentLoadOp::CLEAR;
		LinearColor ClearColor = Vector4(0.f);
		float DepthClearColor = 0.f;
		uint32 StencilClearColor = 0.f;
	};

	class RenderPass;

	struct RenderPassCreateInfo
	{
		String Name;
		Ref<RenderPass> InputPass;
		std::vector<AttachmentInfo> Attachments;
		std::vector<Ref<Image>> ExistingImages;
		uint32 Width;
		uint32 Height;
	};

	class ATHENA_API RenderPass: public RefCounted
	{
	public:
		static Ref<RenderPass> Create(const RenderPassCreateInfo& info);
		virtual ~RenderPass() = default;

		virtual void Begin(const Ref<RenderCommandBuffer>& commandBuffer) = 0;
		virtual void End(const Ref<RenderCommandBuffer>& commandBuffer) = 0;

		virtual void Resize(uint32 width, uint32 height) = 0;

		virtual Ref<Texture2D> GetOutput(uint32 attachmentIndex) const = 0;
		virtual Ref<Texture2D> GetDepthOutput() const = 0;

		virtual uint32 GetColorAttachmentCount() const = 0;
		virtual bool HasDepthAttachment() const = 0;

		const RenderPassCreateInfo& GetInfo() const { return m_Info; }

	protected:
		RenderPassCreateInfo m_Info;
	};
}
