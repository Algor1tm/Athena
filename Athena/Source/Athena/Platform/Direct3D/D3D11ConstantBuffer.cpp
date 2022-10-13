#include "D3D11ConstantBuffer.h"


namespace Athena
{
	D3D11ConstantBuffer::D3D11ConstantBuffer(uint32 size, uint32 binding)
	{
		ATN_CORE_ASSERT(size % 16 == 0 && size <= D3D11_REQ_CONSTANT_BUFFER_ELEMENT_COUNT, "Invalid size for DirectX constant buffer");
		D3D11_BUFFER_DESC bufferDesc;
		bufferDesc.ByteWidth = size;
		bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bufferDesc.MiscFlags = 0;
		bufferDesc.StructureByteStride = 0;

		HRESULT hr = D3D11CurrentContext::Device->CreateBuffer(&bufferDesc, nullptr, m_Buffer.GetAddressOf());
		ATN_CORE_ASSERT(SUCCEEDED(hr), "Failed to create constant buffer!");
		
		ATN_CORE_ASSERT(binding <= D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1, "Invalid binding for DirectX constant buffer");
		D3D11CurrentContext::DeviceContext->VSSetConstantBuffers(binding, 1, m_Buffer.GetAddressOf());
		D3D11CurrentContext::DeviceContext->PSSetConstantBuffers(binding, 1, m_Buffer.GetAddressOf());
	}

	void D3D11ConstantBuffer::SetData(const void* data, uint32 size, uint32 offset)
	{
		D3D11_MAPPED_SUBRESOURCE resource;
		D3D11CurrentContext::DeviceContext->Map(m_Buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
		uint64 destination = (uint64)resource.pData + (uint64)offset;
		memcpy((void*)destination, data, size);
		D3D11CurrentContext::DeviceContext->Unmap(m_Buffer.Get(), 0);
	}
}
