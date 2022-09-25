#pragma once

#include "Athena/Core/Log.h"
#include "Athena/Renderer/Framebuffer.h"
#include "D3D11GraphicsContext.h"


namespace Athena
{
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

	private:
		void DeleteAttachments();
		void AttachColorTexture(ID3D11RenderTargetView** target, DXGI_FORMAT format, uint32 width, uint32 height, SIZE_T index);
		void AttachDepthTexture(ID3D11DepthStencilView** target, DXGI_FORMAT format, uint32 width, uint32 height);

	private:
		FramebufferDescription m_Description;

		std::vector<FramebufferTextureDescription> m_RenderTargetsDescriptions;

		std::vector<Microsoft::WRL::ComPtr<ID3D11Texture2D>> m_RenderTargetsTextures;
		std::vector<Microsoft::WRL::ComPtr<ID3D11RenderTargetView>> m_RenderTargets;
		std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> m_ShaderResourceViews;
		std::vector<Microsoft::WRL::ComPtr<ID3D11Texture2D>> m_ReadableTextures;

		SIZE_T m_ClearColorTargetIndex;

		FramebufferTextureDescription m_DepthStencilDescription = FramebufferTextureFormat::NONE;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_DepthStencil;
	};
}
