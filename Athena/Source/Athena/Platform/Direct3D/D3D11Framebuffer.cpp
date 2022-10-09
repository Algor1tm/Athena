#include "atnpch.h"
#include "D3D11Framebuffer.h"


namespace Athena
{
	static uint32 s_MaxFramebufferSize = 8192;

	static bool IsDepthFormat(FramebufferTextureFormat format)
	{
		switch (format)
		{
		case FramebufferTextureFormat::DEPTH24STENCIL8: return true;
		}

		return false;
	}

	static DXGI_FORMAT AthenaFormatToDXGIFormat(FramebufferTextureFormat format)
	{
		switch (format)
		{
		case FramebufferTextureFormat::RGBA8: return DXGI_FORMAT_R8G8B8A8_UNORM;
		case FramebufferTextureFormat::RED_INTEGER: return DXGI_FORMAT_R32_SINT; 
		case FramebufferTextureFormat::DEPTH24STENCIL8: return DXGI_FORMAT_D24_UNORM_S8_UINT;
		}

		ATN_CORE_ASSERT(false);
		return DXGI_FORMAT_UNKNOWN;
	}

	D3D11Framebuffer::D3D11Framebuffer(const FramebufferDescription& desc)
		: m_Description(desc)
	{
		m_RenderTargetsDescriptions.reserve(desc.Attachments.Attachments.size());

		const auto& attachments = desc.Attachments.Attachments;
		for (SIZE_T i = 0; i < attachments.size(); ++i)
		{
			if (attachments[i].TextureFormat == FramebufferTextureFormat::RGBA8)
				m_ClearColorTargetIndex = i;

			if (!IsDepthFormat(desc.Attachments.Attachments[i].TextureFormat))
				m_RenderTargetsDescriptions.emplace_back(attachments[i].TextureFormat);
			else
				m_DepthStencilDescription = attachments[i].TextureFormat;
		}

		Recreate();
	}

	D3D11Framebuffer::~D3D11Framebuffer()
	{
		
	}

	void D3D11Framebuffer::Recreate()
	{
		if (!m_RenderTargetsDescriptions.empty())
		{
			m_Attachments.resize(m_RenderTargetsDescriptions.size());
			if (IsMultisample())
				m_ResolvedAttachments.resize(m_RenderTargetsDescriptions.size());
			//Attachments
			for (SIZE_T i = 0; i < m_Attachments.size(); ++i)
			{
				switch (m_RenderTargetsDescriptions[i].TextureFormat)
				{
				case FramebufferTextureFormat::RGBA8:
					AttachColorTexture(m_Description.Samples, DXGI_FORMAT_R8G8B8A8_UNORM, m_Description.Width, m_Description.Height, i);
					break;
				case FramebufferTextureFormat::RED_INTEGER:
					AttachColorTexture(m_Description.Samples, DXGI_FORMAT_R32_SINT, m_Description.Width, m_Description.Height, i);
				}
			}
		}

		if (m_DepthStencilDescription.TextureFormat != FramebufferTextureFormat::NONE)
		{
			switch (m_DepthStencilDescription.TextureFormat)
			{
			case FramebufferTextureFormat::DEPTH24STENCIL8:
				AttachDepthTexture(m_Description.Samples, DXGI_FORMAT_D24_UNORM_S8_UINT, m_Description.Width, m_Description.Height);
				break;
			}
		}
	}

	void D3D11Framebuffer::Resize(uint32 width, uint32 height)
	{
		if (width == 0 || height == 0 || width > s_MaxFramebufferSize || height > s_MaxFramebufferSize)
		{
			ATN_CORE_WARN("Attempted to resize Framebuffer to invalid size ({0}, {1})", width, height);
			return;
		}

		m_Description.Width = width;
		m_Description.Height = height;

		DeleteAttachments();
		Recreate();
	}

	void* D3D11Framebuffer::GetColorAttachmentRendererID(SIZE_T index) const
	{ 
		if (IsMultisample())
		{
			ATN_CORE_ASSERT(index < m_ResolvedAttachments.size(), "Subscript out of range!");
			return m_ResolvedAttachments[index].ShaderResourceView.Get();
		}

		ATN_CORE_ASSERT(index < m_Attachments.size(), "Subscript out of range!");
		return m_Attachments[index].ShaderResourceView.Get();
	}

	int D3D11Framebuffer::ReadPixel(SIZE_T attachmentIndex, int x, int y)
	{
		ATN_CORE_ASSERT(attachmentIndex < m_Attachments.size(), "Subscript out of range!");

		D3D11_BOX srcBox;
		srcBox.left = x;
		srcBox.right = srcBox.left + 1;
		srcBox.top = m_Description.Height - y - 1; // Reverse y
		srcBox.bottom = srcBox.top - 1;
		srcBox.front = 0;
		srcBox.back = 1;

		auto source = IsMultisample() ? m_ResolvedAttachments[attachmentIndex].SingleSampledTexture.Get(): m_Attachments[attachmentIndex].RenderTargetTexture.Get();
		D3D11CurrentContext::DeviceContext->CopySubresourceRegion(m_Attachments[attachmentIndex].ReadableTexture.Get(), 0, 0, 0, 0, source, 0, &srcBox);

		D3D11_MAPPED_SUBRESOURCE msr;
		D3D11CurrentContext::DeviceContext->Map(m_Attachments[attachmentIndex].ReadableTexture.Get(), 0, D3D11_MAP_READ, 0, &msr);
		void* pixel = msr.pData;
		D3D11CurrentContext::DeviceContext->Unmap(m_Attachments[attachmentIndex].ReadableTexture.Get(), 0);

		return *reinterpret_cast<int*>(pixel);
	}

	void D3D11Framebuffer::ClearAttachment(SIZE_T attachmentIndex, int value)
	{
		ATN_CORE_ASSERT(attachmentIndex < m_Attachments.size(), "Subscript out of range!");

		float color[4] = { (float)value, (float)value, (float)value, (float)value };
		D3D11CurrentContext::DeviceContext->ClearRenderTargetView(m_Attachments[attachmentIndex].RenderTargetView.Get(), color);
	}

	void D3D11Framebuffer::ClearColorAndDepth(const LinearColor& color)
	{
		std::vector<ID3D11RenderTargetView*> views(m_Attachments.size());
		for (SIZE_T i = 0; i < m_Attachments.size(); ++i)
		{
			views[i] = m_Attachments[i].RenderTargetView.Get();
		}

		D3D11_VIEWPORT viewport;
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Width = (float)m_Description.Width;
		viewport.Height = (float)m_Description.Height;
		viewport.MinDepth = 0;
		viewport.MaxDepth = 0;

		D3D11CurrentContext::DeviceContext->RSSetViewports(1, &viewport);

		D3D11CurrentContext::DeviceContext->OMSetRenderTargets((UINT)views.size(), views.data(), m_DepthStencil.Get());

		D3D11CurrentContext::DeviceContext->ClearRenderTargetView(m_Attachments[m_ClearColorTargetIndex].RenderTargetView.Get(), color.Data());
		D3D11CurrentContext::DeviceContext->ClearDepthStencilView(m_DepthStencil.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);
	}


	void D3D11Framebuffer::ResolveMutlisampling()
	{
		if (IsMultisample())
		{
			for (SIZE_T i = 0; i < m_RenderTargetsDescriptions.size(); ++i)
			{
				D3D11CurrentContext::DeviceContext->ResolveSubresource(m_ResolvedAttachments[i].SingleSampledTexture.Get(), 0,
					m_Attachments[i].RenderTargetTexture.Get(), 0,
					AthenaFormatToDXGIFormat(m_RenderTargetsDescriptions[i].TextureFormat));
			}
		}
	}

	void D3D11Framebuffer::DeleteAttachments()
	{
		for (SIZE_T i = 0; i < m_RenderTargetsDescriptions.size(); ++i)
		{
			m_Attachments[i].RenderTargetView->Release();
			m_Attachments[i].RenderTargetTexture->Release();
			m_Attachments[i].ReadableTexture->Release();

			if (IsMultisample())
			{
				m_ResolvedAttachments[i].SingleSampledTexture->Release();
				m_ResolvedAttachments[i].ShaderResourceView->Release();
			}
			else
			{
				m_Attachments[i].ShaderResourceView->Release();
			}
		}

		m_DepthStencil->Release();
	}

	void D3D11Framebuffer::AttachColorTexture(uint32 samples, DXGI_FORMAT format, uint32 width, uint32 height, SIZE_T index)
	{
		D3D11_TEXTURE2D_DESC colorTextureDesc;
		colorTextureDesc.Width = width;
		colorTextureDesc.Height = height;
		colorTextureDesc.MipLevels = 1;
		colorTextureDesc.ArraySize = 1;
		colorTextureDesc.Format = format;
		colorTextureDesc.SampleDesc.Count = samples;
		colorTextureDesc.SampleDesc.Quality = 0;
		colorTextureDesc.Usage = D3D11_USAGE_DEFAULT;
		colorTextureDesc.BindFlags = IsMultisample() ? D3D11_BIND_RENDER_TARGET : D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		colorTextureDesc.CPUAccessFlags = 0;
		colorTextureDesc.MiscFlags = 0;

		HRESULT hr = D3D11CurrentContext::Device->CreateTexture2D(&colorTextureDesc, nullptr, m_Attachments[index].RenderTargetTexture.GetAddressOf());
		ATN_CORE_ASSERT(SUCCEEDED(hr), "Failed to create color texture!");

		hr = D3D11CurrentContext::Device->CreateRenderTargetView(m_Attachments[index].RenderTargetTexture.Get(), nullptr, m_Attachments[index].RenderTargetView.GetAddressOf());
		ATN_CORE_ASSERT(SUCCEEDED(hr), "Failed to create color view!");

		if (IsMultisample())
		{
			colorTextureDesc.SampleDesc.Count = 1;
			colorTextureDesc.SampleDesc.Quality = 0;
			colorTextureDesc.Usage = D3D11_USAGE_DEFAULT;
			colorTextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

			hr = D3D11CurrentContext::Device->CreateTexture2D(&colorTextureDesc, nullptr, m_ResolvedAttachments[index].SingleSampledTexture.GetAddressOf());
			ATN_CORE_ASSERT(SUCCEEDED(hr), "Failed to create single sampled texture!");

			hr = D3D11CurrentContext::Device->CreateShaderResourceView(m_ResolvedAttachments[index].SingleSampledTexture.Get(), nullptr, m_ResolvedAttachments[index].ShaderResourceView.GetAddressOf());
			ATN_CORE_ASSERT(SUCCEEDED(hr), "Failed to create shader resource view!");
		}
		else
		{
			hr = D3D11CurrentContext::Device->CreateShaderResourceView(m_Attachments[index].RenderTargetTexture.Get(), nullptr, m_Attachments[index].ShaderResourceView.GetAddressOf());
			ATN_CORE_ASSERT(SUCCEEDED(hr), "Failed to create shader resource view!");
		}

		colorTextureDesc.Width = 1;
		colorTextureDesc.Height = 1;
		colorTextureDesc.SampleDesc.Count = 1;
		colorTextureDesc.SampleDesc.Quality = 0;
		colorTextureDesc.Usage = D3D11_USAGE_STAGING;
		colorTextureDesc.BindFlags = 0;
		colorTextureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

		hr = D3D11CurrentContext::Device->CreateTexture2D(&colorTextureDesc, nullptr, m_Attachments[index].ReadableTexture.GetAddressOf());
		ATN_CORE_ASSERT(SUCCEEDED(hr), "Failed to create readable texture!");

	}

	void D3D11Framebuffer::AttachDepthTexture(uint32 samples, DXGI_FORMAT format, uint32 width, uint32 height)
	{
		ID3D11Texture2D* depthTexture;

		D3D11_TEXTURE2D_DESC depthStencilTextureDesc;
		depthStencilTextureDesc.Width = width;
		depthStencilTextureDesc.Height = height;
		depthStencilTextureDesc.MipLevels = 1;
		depthStencilTextureDesc.ArraySize = 1;
		depthStencilTextureDesc.Format = format;
		depthStencilTextureDesc.SampleDesc.Count = samples;
		depthStencilTextureDesc.SampleDesc.Quality = 0;
		depthStencilTextureDesc.Usage = D3D11_USAGE_DEFAULT;
		depthStencilTextureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		depthStencilTextureDesc.CPUAccessFlags = 0;
		depthStencilTextureDesc.MiscFlags = 0;

		HRESULT hr = D3D11CurrentContext::Device->CreateTexture2D(&depthStencilTextureDesc, nullptr, &depthTexture);
		ATN_CORE_ASSERT(SUCCEEDED(hr), "Failed to create depth stencil texture!");

		hr = D3D11CurrentContext::Device->CreateDepthStencilView(depthTexture, nullptr, m_DepthStencil.GetAddressOf());
		ATN_CORE_ASSERT(SUCCEEDED(hr), "Failed to create depth stencil view!");

		depthTexture->Release();
	}
}
