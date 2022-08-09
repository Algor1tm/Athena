#pragma once

#include "Athena/Renderer/Buffer.h"


namespace Athena
{
	class ATHENA_API OpenGLVertexBuffer: public VertexBuffer
	{
	public:
		OpenGLVertexBuffer(float* vertices, uint32 size);
		OpenGLVertexBuffer(uint32 size);
		~OpenGLVertexBuffer();

		void Bind() const override;
		void UnBind() const override;

		void SetData(const void* data, uint32 size) override;

		inline void SetLayout(const BufferLayout& layout) override { m_Layout = layout; }
		inline const BufferLayout& GetLayout() const override { return m_Layout; }
	private:
		RendererID m_RendererID = 0;
		BufferLayout m_Layout;
	};


	class ATHENA_API OpenGLIndexBuffer : public IndexBuffer
	{
	public:
		OpenGLIndexBuffer(uint32* indices, uint32 ñount);
		~OpenGLIndexBuffer();

		void Bind() const override;
		void UnBind() const override;

		inline uint32 GetCount() const override { return m_Count; };
	private:
		RendererID m_RendererID = 0;
		uint32 m_Count;
	};
}
