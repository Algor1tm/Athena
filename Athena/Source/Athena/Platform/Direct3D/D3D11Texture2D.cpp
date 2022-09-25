#include "atnpch.h"
#include "D3D11Texture2D.h"

#include <stb_image/stb_image.h>


namespace Athena
{
	D3D11Texture2D::D3D11Texture2D(uint32 width, uint32 height)
		: m_Width(width), m_Height(height)
	{
		D3D11_TEXTURE2D_DESC texDesc;
		texDesc.Width = m_Width;
		texDesc.Height = m_Height;
		texDesc.MipLevels = 1;
		texDesc.ArraySize = 1;
		texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Usage = D3D11_USAGE_DYNAMIC;
		texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		texDesc.MiscFlags = 0;

		HRESULT hr = D3D11CurrentContext::Device->CreateTexture2D(&texDesc, nullptr, m_Texture2D.GetAddressOf());
		ATN_CORE_ASSERT(SUCCEEDED(hr), "Failed to create texture2D!");

		hr = D3D11CurrentContext::Device->CreateShaderResourceView(m_Texture2D.Get(), nullptr, m_ShaderResourceView.GetAddressOf());
		ATN_CORE_ASSERT(SUCCEEDED(hr), "Failed to create shader resource view!");
	}

	D3D11Texture2D::D3D11Texture2D(const Filepath& path)
		: m_Path(path)
	{
		int width, height, channels;
		stbi_set_flip_vertically_on_load(1);

		unsigned char* data;
		data = stbi_load(path.string().data(), &width, &height, &channels, 0);

		int pitch = width * channels;

		if (data)
		{
			m_Width = width;
			m_Height = height;
			m_IsLoaded = true;

			DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
			if (channels == 3)
			{
				format = DXGI_FORMAT_B8G8R8X8_UNORM;		// TODO: FIX
			}
			else if(channels == 4)
			{
				format = DXGI_FORMAT_R8G8B8A8_UNORM;
			}

			D3D11_TEXTURE2D_DESC texDesc;
			texDesc.Width = width;
			texDesc.Height = height;
			texDesc.MipLevels = 1;
			texDesc.ArraySize = 1;
			texDesc.Format = format;
			texDesc.SampleDesc.Count = 1;
			texDesc.SampleDesc.Quality = 0;
			texDesc.Usage = D3D11_USAGE_IMMUTABLE;
			texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			texDesc.CPUAccessFlags = 0;
			texDesc.MiscFlags = 0;

			D3D11_SUBRESOURCE_DATA initialData;
			initialData.pSysMem = data;
			initialData.SysMemPitch = pitch;
			initialData.SysMemSlicePitch = 0;

			HRESULT hr = D3D11CurrentContext::Device->CreateTexture2D(&texDesc, &initialData, m_Texture2D.GetAddressOf());
			ATN_CORE_ASSERT(SUCCEEDED(hr), "Failed to create texture2D!");

			hr = D3D11CurrentContext::Device->CreateShaderResourceView(m_Texture2D.Get(), nullptr, m_ShaderResourceView.GetAddressOf());
			ATN_CORE_ASSERT(SUCCEEDED(hr), "Failed to create shader resource view!");

			stbi_image_free(data);
		}
		else
		{
			ATN_CORE_ERROR("Failed to load image for Texture2D! (path = '{0}')", path);
		}
	}

	D3D11Texture2D::~D3D11Texture2D()
	{

	}

	void D3D11Texture2D::SetData(const void* data, uint32 size)
	{
		D3D11_MAPPED_SUBRESOURCE resource;
		D3D11CurrentContext::DeviceContext->Map(m_Texture2D.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
		memcpy(resource.pData, data, size);
		D3D11CurrentContext::DeviceContext->Unmap(m_Texture2D.Get(), 0);
	}

	void D3D11Texture2D::Bind(uint32 slot) const
	{
		D3D11CurrentContext::DeviceContext->PSSetShaderResources(slot, 1, m_ShaderResourceView.GetAddressOf());
	}
}
