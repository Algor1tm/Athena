#pragma once

#include "Athena/Renderer/RendererAPI.h"
#include "D3D11GraphicsContext.h"


namespace Athena
{
	class ATHENA_API D3D11RendererAPI : public RendererAPI
	{
	public:
		virtual void Init() override;
		virtual void SetViewport(uint32 x, uint32 y, uint32 width, uint32 height) override;
		virtual void Clear(const LinearColor& color) override;
		virtual void DrawTriangles(const Ref<VertexBuffer>& vertexBuffer, uint32 indexCount = 0) override;
		virtual void DrawLines(const Ref<VertexBuffer>& vertexBuffer, uint32 vertexCount = 0) override;

		virtual void BindFramebuffer(const Ref<Framebuffer>& framebuffer) override;
		virtual void UnBindFramebuffer() override;

	private:
		void ClearRenderTargetView();
		void CreateRenderTargetView();
		void ClearDepthStencilView();
		void CreateDepthStencilView(uint32 width, uint32 height);

	private:
		Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_DepthStencilState;
		Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_RasterizerState;
		Microsoft::WRL::ComPtr<ID3D11BlendState> m_BlendState;
		Microsoft::WRL::ComPtr<ID3D11SamplerState> m_SamplerState;

		Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_DepthStencilView;
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_BackBufferView;

		Ref<Framebuffer> m_OutputFramebuffer = nullptr;
	};
}
