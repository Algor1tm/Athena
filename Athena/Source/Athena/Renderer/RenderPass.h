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

	struct RenderPassAttachment
	{
		RenderPassAttachment() = default;
		RenderPassAttachment(const String& name, ImageFormat format)
		{
			uint32 usage = ImageUsage::ATTACHMENT | ImageUsage::SAMPLED | ImageUsage::TRANSFER_SRC;

			Texture2DCreateInfo texInfo;
			texInfo.Name = name;
			texInfo.Format = format;
			texInfo.Usage = (ImageUsage)usage;
			texInfo.Width = 1;
			texInfo.Height = 1;
			texInfo.Layers = 1;
			texInfo.MipLevels = 1;
			texInfo.SamplerInfo.MinFilter = TextureFilter::LINEAR;
			texInfo.SamplerInfo.MagFilter = TextureFilter::LINEAR;
			texInfo.SamplerInfo.Wrap = TextureWrap::REPEAT;

			Texture = Texture2D::Create(texInfo);
			LoadOp = AttachmentLoadOp::CLEAR;
		}

		RenderPassAttachment(const Ref<Texture2D>& texture)
		{
			Texture = texture;
			LoadOp = AttachmentLoadOp::LOAD;
		}

		Ref<Texture2D> Texture;
		AttachmentLoadOp LoadOp = AttachmentLoadOp::CLEAR;
		LinearColor ClearColor = { 0.f, 0.f, 0.f, 1.0f };
		float DepthClearColor = 1.f;
		uint32 StencilClearColor = 1.f;
	};

	class RenderPass;

	struct RenderPassCreateInfo
	{
		String Name;
		Ref<RenderPass> InputPass;
		uint32 Width;
		uint32 Height;
		LinearColor DebugColor = LinearColor(0.f, 0.f, 0.f, 0.f);
	};

	class ATHENA_API RenderPass: public RefCounted
	{
	public:
		static Ref<RenderPass> Create(const RenderPassCreateInfo& info);
		virtual ~RenderPass() = default;

		virtual void Begin(const Ref<RenderCommandBuffer>& commandBuffer) = 0;
		virtual void End(const Ref<RenderCommandBuffer>& commandBuffer) = 0;

		virtual void Resize(uint32 width, uint32 height) = 0;
		virtual void Bake() = 0;

		void SetOutput(const RenderPassAttachment& attachment);
		Ref<Texture2D> GetOutput(const String& name) const;
		Ref<Texture2D> GetDepthOutput() const;
		const auto& GetAllOutputs() const { return m_Outputs; }

		uint32 GetColorAttachmentCount() const;
		bool HasDepthAttachment() const;

		const RenderPassCreateInfo& GetInfo() const { return m_Info; }

	protected:
		RenderPassCreateInfo m_Info;
		std::vector<RenderPassAttachment> m_Outputs;
	};
}
