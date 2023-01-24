#pragma once

#include "Athena/Core/Log.h"
#include "Athena/Renderer/Framebuffer.h"
#include "D3D11GraphicsContext.h"


namespace Athena
{
	struct FramebufferAttachment
	{
		Microsoft::WRL::ComPtr<ID3D11Texture2D> RenderTargetTexture = nullptr;
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RenderTargetView = nullptr;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ShaderResourceView = nullptr;
		Microsoft::WRL::ComPtr<ID3D11Texture2D> ReadableTexture = nullptr;
	};

	struct ResolvedFramebufferAttachment
	{
		Microsoft::WRL::ComPtr<ID3D11Texture2D> SingleSampledTexture = nullptr;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ShaderResourceView = nullptr;
	};


	class ATHENA_API D3D11Framebuffer: public Framebuffer
	{
	public:
		D3D11Framebuffer(const FramebufferDescription& desc);
		virtual ~D3D11Framebuffer();

		void Recreate();

		virtual void Resize(uint32 width, uint32 height) override;

		virtual const FramebufferDescription& GetDescription() const override { return m_Description; }
		virtual void* GetColorAttachmentRendererID(SIZE_T index = 0) const override;

		virtual int ReadPixel(SIZE_T attachmentIndex, int x, int y) override;
		virtual void ClearAttachment(SIZE_T attachmentIndex, int value) override;

		virtual void ClearColorAndDepth(const LinearColor& color) override;
		virtual void ReplaceAttachment(SIZE_T attachmentIndex, TextureTarget textureTarget, void* rendererID) override {};

		virtual void ResolveMutlisampling() override;

	private:
		void DeleteAttachments();
		void AttachColorTexture(uint32 samples, DXGI_FORMAT format, uint32 width, uint32 height, SIZE_T index);
		void AttachDepthTexture(uint32 samples, DXGI_FORMAT format, uint32 width, uint32 height);

		bool IsMultisample() const { return m_Description.Samples > 1; }

	private:
		FramebufferDescription m_Description;

		std::vector<FramebufferTextureDescription> m_RenderTargetsDescriptions;
		std::vector<FramebufferAttachment> m_Attachments;
		std::vector<ResolvedFramebufferAttachment> m_ResolvedAttachments; // if not multisample - invalid

		SIZE_T m_ClearColorTargetIndex;

		FramebufferTextureDescription m_DepthStencilDescription = TextureFormat::NONE;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_DepthStencil;
	};
}
