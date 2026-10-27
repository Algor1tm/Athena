#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/RHI/D3D12/D3D12RHI.h"


namespace Athena
{
	class ATHENA_API D3D12VertexBuffer : public VertexBuffer
	{
	public:
		D3D12VertexBuffer(const String& name, uint64 size);
		virtual ~D3D12VertexBuffer();

		virtual void Init() override;
		virtual void Upload(const Buffer& buffer, uint64 offset) override;
		virtual void Resize(uint64 size) override;

	private:
		void CleanUp();

	private:
		D3D12Resource m_Buffer;
	};

	class ATHENA_API D3D12IndexBuffer : public IndexBuffer
	{
	public:
		D3D12IndexBuffer(const String& name, uint64 size, IndexSize indexSize);
		virtual ~D3D12IndexBuffer();

		virtual void Init() override;
		virtual void Upload(const Buffer& buffer, uint64 offset) override;
		virtual void Resize(uint64 size) override;

	private:
		void CleanUp();

	private:
		D3D12Resource m_Buffer;
	};
}
