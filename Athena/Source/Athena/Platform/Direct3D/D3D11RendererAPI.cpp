#include "atnpch.h"
#include "D3D11RendererAPI.h"


namespace Athena
{
	void D3D11RendererAPI::Init()
	{
		CreateRenderTargetView();

		D3D11_RASTERIZER_DESC rasterizerDesc;
		rasterizerDesc.FillMode = D3D11_FILL_MODE::D3D11_FILL_SOLID;
		rasterizerDesc.CullMode = D3D11_CULL_MODE::D3D11_CULL_NONE;
		rasterizerDesc.FrontCounterClockwise = TRUE;
		rasterizerDesc.DepthBias = 0;
		rasterizerDesc.DepthBiasClamp = 0;
		rasterizerDesc.SlopeScaledDepthBias = 0;
		rasterizerDesc.DepthClipEnable = FALSE;
		rasterizerDesc.ScissorEnable = FALSE;
		rasterizerDesc.MultisampleEnable = TRUE;
		rasterizerDesc.AntialiasedLineEnable = FALSE;
		
		HRESULT hr = D3D11CurrentContext::Device->CreateRasterizerState(&rasterizerDesc, m_RasterizerState.GetAddressOf());
		ATN_CORE_ASSERT(SUCCEEDED(hr), "Failed to create rasterizer state!");
		
		
		D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
		ZeroMemory(&depthStencilDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
		depthStencilDesc.DepthEnable = true;
		depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK::D3D11_DEPTH_WRITE_MASK_ALL;
		depthStencilDesc.DepthFunc = D3D11_COMPARISON_FUNC::D3D11_COMPARISON_LESS_EQUAL;
		
		hr = D3D11CurrentContext::Device->CreateDepthStencilState(&depthStencilDesc, m_DepthStencilState.GetAddressOf());
		ATN_CORE_ASSERT(SUCCEEDED(hr), "Failed to create depth stencil state!");
		
		
		D3D11_BLEND_DESC blendDesc;
		ZeroMemory(&blendDesc, sizeof(D3D11_BLEND_DESC));
		
		D3D11_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc;
		ZeroMemory(&renderTargetBlendDesc, sizeof(D3D11_RENDER_TARGET_BLEND_DESC));
		renderTargetBlendDesc.BlendEnable = true;
		renderTargetBlendDesc.SrcBlend = D3D11_BLEND::D3D11_BLEND_SRC_ALPHA;
		renderTargetBlendDesc.DestBlend = D3D11_BLEND::D3D11_BLEND_INV_SRC_ALPHA;
		renderTargetBlendDesc.BlendOp = D3D11_BLEND_OP::D3D11_BLEND_OP_ADD;
		renderTargetBlendDesc.SrcBlendAlpha = D3D11_BLEND::D3D11_BLEND_ONE;
		renderTargetBlendDesc.DestBlendAlpha = D3D11_BLEND::D3D11_BLEND_ZERO;
		renderTargetBlendDesc.BlendOpAlpha = D3D11_BLEND_OP::D3D11_BLEND_OP_ADD;
		renderTargetBlendDesc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE::D3D11_COLOR_WRITE_ENABLE_ALL;
		
		blendDesc.RenderTarget[0] = renderTargetBlendDesc;
		hr = D3D11CurrentContext::Device->CreateBlendState(&blendDesc, m_BlendState.GetAddressOf());
		ATN_CORE_ASSERT(SUCCEEDED(hr), "Failed to create blend state!");
		
		
		D3D11_SAMPLER_DESC samplerDesc;
		ZeroMemory(&samplerDesc, sizeof(samplerDesc));
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		samplerDesc.MinLOD = 0;
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
		
		hr = D3D11CurrentContext::Device->CreateSamplerState(&samplerDesc, m_SamplerState.GetAddressOf());
		ATN_CORE_ASSERT(SUCCEEDED(hr), "Failed to create blend state!");
	}

	void D3D11RendererAPI::SetViewport(uint32 x, uint32 y, uint32 width, uint32 height)
	{
		ClearRenderTargetView();
		D3D11CurrentContext::SwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
		CreateRenderTargetView();

		ClearDepthStencilView();
		CreateDepthStencilView(width, height);
		
		D3D11_VIEWPORT viewport;
		viewport.TopLeftX = (float)x;
		viewport.TopLeftY = (float)y;
		viewport.Width = (float)width;
		viewport.Height = (float)height;
		viewport.MinDepth = 0;
		viewport.MaxDepth = 0;

		D3D11CurrentContext::DeviceContext->RSSetViewports(1, &viewport);
	}

	void D3D11RendererAPI::Clear(const LinearColor& color)
	{
		if (m_OutputFramebuffer != nullptr)
		{
			m_OutputFramebuffer->ClearColorAndDepth(color);
		}
		else
		{
			D3D11CurrentContext::DeviceContext->OMSetRenderTargets(1, m_BackBufferView.GetAddressOf(), nullptr);
			D3D11CurrentContext::DeviceContext->ClearRenderTargetView(m_BackBufferView.Get(), color.Data());
			D3D11CurrentContext::DeviceContext->ClearDepthStencilView(m_DepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);
		}

		D3D11CurrentContext::DeviceContext->RSSetState(m_RasterizerState.Get());
		D3D11CurrentContext::DeviceContext->OMSetDepthStencilState(m_DepthStencilState.Get(), 0);
		D3D11CurrentContext::DeviceContext->OMSetBlendState(m_BlendState.Get(), nullptr, 0xFFFFFFFF);
		D3D11CurrentContext::DeviceContext->PSSetSamplers(0, 1, m_SamplerState.GetAddressOf());
	}

	void D3D11RendererAPI::DrawTriangles(const Ref<VertexBuffer>& vertexBuffer, uint32 indexCount)
	{
		vertexBuffer->Bind();
		D3D11CurrentContext::DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		uint32 count = indexCount ? indexCount : vertexBuffer->GetIndexBuffer()->GetCount();
		D3D11CurrentContext::DeviceContext->DrawIndexed(count, 0, 0);
	}

	void D3D11RendererAPI::DrawLines(const Ref<VertexBuffer>& vertexBuffer, uint32 vertexCount)
	{
		vertexBuffer->Bind();
		D3D11CurrentContext::DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_LINELIST);

		D3D11CurrentContext::DeviceContext->Draw(vertexCount, 0);
	}

	void D3D11RendererAPI::BindFramebuffer(const Ref<Framebuffer>& framebuffer)
	{
		m_OutputFramebuffer = framebuffer;
	}

	void D3D11RendererAPI::UnBindFramebuffer()
	{
		D3D11CurrentContext::DeviceContext->OMSetRenderTargets(1, m_BackBufferView.GetAddressOf(), nullptr);
		m_OutputFramebuffer = nullptr;
	}


	void D3D11RendererAPI::ClearRenderTargetView()
	{
		if (m_BackBufferView != nullptr)
		{
			m_BackBufferView->Release();
		}
	}

	void D3D11RendererAPI::CreateRenderTargetView()
	{
		Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
		D3D11CurrentContext::SwapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf()));
		D3D11CurrentContext::Device->CreateRenderTargetView(backBuffer.Get(), nullptr, m_BackBufferView.GetAddressOf());
	}

	void D3D11RendererAPI::ClearDepthStencilView()
	{
		if (m_DepthStencilView != nullptr)
		{
			m_DepthStencilView->Release();
		}
	}

	void D3D11RendererAPI::CreateDepthStencilView(uint32 width, uint32 height)
	{
		ID3D11Texture2D* depthStencilTexture;

		D3D11_TEXTURE2D_DESC depthStencilTextureDesc;
		depthStencilTextureDesc.Width = width;
		depthStencilTextureDesc.Height = height;
		depthStencilTextureDesc.MipLevels = 1;
		depthStencilTextureDesc.ArraySize = 1;
		depthStencilTextureDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthStencilTextureDesc.SampleDesc.Count = 1;
		depthStencilTextureDesc.SampleDesc.Quality = 0;
		depthStencilTextureDesc.Usage = D3D11_USAGE_DEFAULT;
		depthStencilTextureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		depthStencilTextureDesc.CPUAccessFlags = 0;
		depthStencilTextureDesc.MiscFlags = 0;

		HRESULT hr = D3D11CurrentContext::Device->CreateTexture2D(&depthStencilTextureDesc, nullptr, &depthStencilTexture);
		ATN_CORE_ASSERT(SUCCEEDED(hr), "Failed to create depth stencil texture!");

		hr = D3D11CurrentContext::Device->CreateDepthStencilView(depthStencilTexture, nullptr, m_DepthStencilView.GetAddressOf());
		ATN_CORE_ASSERT(SUCCEEDED(hr), "Failed to create depth stencil view!");

		depthStencilTexture->Release();
	}
}
