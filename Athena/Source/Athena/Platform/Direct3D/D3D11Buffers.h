#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Platform/Direct3D/D3D11GraphicsContext.h"

#include "Athena/Renderer/GPUBuffers.h"


namespace Athena
{
	class ATHENA_API D3D11VertexBuffer : public VertexBuffer
	{
	public:
		D3D11VertexBuffer(const VertexBufferDescription& desc);
		virtual ~D3D11VertexBuffer();

		virtual void Bind() const override;
		virtual void UnBind() const override;

		virtual void SetData(const void* data, uint32 size) override;

		virtual const Ref<IndexBuffer>& GetIndexBuffer() const override { return m_IndexBuffer; }

	private:
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_Buffer;
		Ref<IndexBuffer> m_IndexBuffer;
		uint32 m_Stride;
	};


	class ATHENA_API D3D11IndexBuffer : public IndexBuffer
	{
	public:
		D3D11IndexBuffer(uint32* indices, uint32 count);
		virtual ~D3D11IndexBuffer();

		virtual void Bind() const override;
		virtual void UnBind() const override;

		virtual uint32 GetCount() const override { return m_Count; };

	private:
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_Buffer;
		uint32 m_Count;
	};


	class ATHENA_API D3D11ConstantBuffer : public ConstantBuffer
	{
	public:
		D3D11ConstantBuffer(uint32 size, uint32 binding);

		virtual void SetData(const void* data, uint32 size, uint32 offset = 0) override;

	private:
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_Buffer;
	};
}
