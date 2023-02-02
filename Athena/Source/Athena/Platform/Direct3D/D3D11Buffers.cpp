#include "D3D11Buffers.h"


namespace Athena
{
	////////////////////////////////////////////////////////
	///////////////////// VertexBuffer /////////////////////
	////////////////////////////////////////////////////////

	D3D11VertexBuffer::D3D11VertexBuffer(const VertexBufferDescription& desc)
	{
		bool staticUsage = desc.Usage == BufferUsage::STATIC;

		D3D11_BUFFER_DESC bufferDesc;
		bufferDesc.ByteWidth = desc.Size;
		bufferDesc.Usage = staticUsage ? D3D11_USAGE_DEFAULT : D3D11_USAGE_DYNAMIC;
		bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bufferDesc.CPUAccessFlags = staticUsage ? 0 : D3D11_CPU_ACCESS_WRITE;
		bufferDesc.MiscFlags = 0;
		bufferDesc.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA initData;
		initData.pSysMem = desc.Data;
		initData.SysMemPitch = 0;
		initData.SysMemSlicePitch = 0;

		HRESULT hr = D3D11CurrentContext::Device->CreateBuffer(&bufferDesc, staticUsage ? &initData : nullptr, m_Buffer.GetAddressOf());
		ATN_CORE_ASSERT(SUCCEEDED(hr), "Failed to create vertex buffer!");

		ATN_CORE_ASSERT(desc.pBufferLayout->GetElements().size(), "Invalid buffer layout!");
		m_Stride = desc.pBufferLayout->GetStride();
		m_IndexBuffer = desc.pIndexBuffer;
	}

	D3D11VertexBuffer::~D3D11VertexBuffer()
	{

	}

	void D3D11VertexBuffer::Bind() const
	{
		const uint32 offset = 0;
		D3D11CurrentContext::DeviceContext->IASetVertexBuffers(0, 1, m_Buffer.GetAddressOf(), &m_Stride, &offset);
		m_IndexBuffer->Bind();
	}

	void D3D11VertexBuffer::UnBind() const
	{

	}

	void D3D11VertexBuffer::SetData(const void* data, uint32 size)
	{
		D3D11_MAPPED_SUBRESOURCE resource;
		D3D11CurrentContext::DeviceContext->Map(m_Buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
		memcpy(resource.pData, data, size);
		D3D11CurrentContext::DeviceContext->Unmap(m_Buffer.Get(), 0);
	}

	/////////////////////////////////////////////////////////
	///////////////////// IndexBuffer ///////////////////////
	/////////////////////////////////////////////////////////

	D3D11IndexBuffer::D3D11IndexBuffer(uint32* indices, uint32 count)
		: m_Count(count)
	{
		D3D11_BUFFER_DESC bufferDesc;
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		bufferDesc.ByteWidth = sizeof(uint32) * count;
		bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bufferDesc.CPUAccessFlags = 0;
		bufferDesc.MiscFlags = 0;
		bufferDesc.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA InitData;
		InitData.pSysMem = indices;
		InitData.SysMemPitch = 0;
		InitData.SysMemSlicePitch = 0;

		HRESULT hr = D3D11CurrentContext::Device->CreateBuffer(&bufferDesc, &InitData, &m_Buffer);
		ATN_CORE_ASSERT(SUCCEEDED(hr), "Failed to crate index buffer!");
	}

	D3D11IndexBuffer::~D3D11IndexBuffer()
	{

	}

	void D3D11IndexBuffer::Bind() const
	{
		D3D11CurrentContext::DeviceContext->IASetIndexBuffer(m_Buffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	}

	void D3D11IndexBuffer::UnBind() const
	{

	}

	/////////////////////////////////////////////////////////
	///////////////////// ConstantBuffer /////////////////////
	/////////////////////////////////////////////////////////

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
