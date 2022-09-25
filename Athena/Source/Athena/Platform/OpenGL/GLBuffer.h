#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Buffer.h"


namespace Athena
{
	class ATHENA_API GLVertexBuffer: public VertexBuffer
	{
	public:
		GLVertexBuffer(const VertexBufferDescription& desc);
		virtual ~GLVertexBuffer();

		virtual void Bind() const override;
		virtual void UnBind() const override;

		virtual void SetData(const void* data, uint32 size) override;

		virtual const Ref<IndexBuffer>& GetIndexBuffer() const override { return m_IndexBuffer; }

	private:
		void SetLayout(const BufferLayout& layout);
		void SetIndexBuffer(const Ref<IndexBuffer>& indexBuffer);

	private:
		uint32 m_VertexArrayRendererID = 0;
		uint32 m_RendererID = 0;

		Ref<IndexBuffer> m_IndexBuffer;
	};


	class ATHENA_API GLIndexBuffer : public IndexBuffer
	{
	public:
		GLIndexBuffer(uint32* indices, uint32 count);
		virtual ~GLIndexBuffer();

		virtual void Bind() const override;
		virtual void UnBind() const override;

		virtual uint32 GetCount() const override { return m_Count; };

	private:
		uint32 m_RendererID = 0;
		uint32 m_Count = 0;
	};
}
