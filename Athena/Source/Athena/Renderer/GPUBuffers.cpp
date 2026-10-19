#include "GPUBuffers.h"
#include "Athena/Renderer/RenderThread.h"
#include "Athena/Renderer/RHI/RHI.h"


namespace Athena
{
	void VertexMemoryLayout::CalculateOffsetsAndStride()
	{
		uint32 offset = 0;
		for (auto& elem : m_Elements)
		{
			ensure(GetFormatSupportProperties(elem.ElementFormat).IsVertexBufferSupported, "Format is not supported for vertex buffers!");
			uint32 elemSize = GetFormatProperties(elem.ElementFormat).BytesPerPixel;

			elem.Offset = offset;
			offset += elemSize;
			m_Stride += elemSize;
		}
	}

	Ref<VertexBuffer> VertexBuffer::Create(const String& name, uint64 size, Buffer* vertices)
	{
		TRACY_PROFILE_FUNC();

		Ref<VertexBuffer> vertexBuffer = RHI::Get()->CreateVertexBuffer(name, size);

		if (vertices)
		{
			Buffer verticesCopy = Buffer::Copy(*vertices);
			check(vertexBuffer->m_Size >= verticesCopy.Size() && verticesCopy.Data() != nullptr, "Invalid initialization buffer for VertexBuffer!");

			ENQUEUE_RENDER_COMMAND([vertexBuffer, verticesCopyRT = std::move(verticesCopy)]()
				{
					vertexBuffer->Init();
					vertexBuffer->Upload(verticesCopyRT);
				});
		}
		else
		{
			ENQUEUE_RENDER_COMMAND([vertexBuffer]()
				{
					vertexBuffer->Init();
				});
		}

		return vertexBuffer;
	}

	VertexBuffer::VertexBuffer(const String& name, uint64 size)
		: m_Name(name), 
		m_Size(size)
	{

	}

	void VertexBuffer::Resize(uint64 size)
	{
		m_Size = size;
	}

	Ref<IndexBuffer> IndexBuffer::Create(const String& name, uint64 size, Buffer* indices, IndexSize indexSize)
	{
		TRACY_PROFILE_FUNC();

		Ref<IndexBuffer> indexBuffer = RHI::Get()->CreateIndexBuffer(name, size, indexSize);

		if (indices)
		{
			Buffer indicesCopy = Buffer::Copy(*indices);
			check(indexBuffer->m_Size >= indicesCopy.Size() && indicesCopy.Data() != nullptr, "Invalid initialization buffer for IndexBuffer!");

			ENQUEUE_RENDER_COMMAND([indexBuffer, indicesCopyRT = std::move(indicesCopy)]()
				{
					indexBuffer->Init();
					indexBuffer->Upload(indicesCopyRT);
				});
		}
		else
		{
			ENQUEUE_RENDER_COMMAND([indexBuffer]()
				{
					indexBuffer->Init();
				});
		}

		return indexBuffer;
	}

	IndexBuffer::IndexBuffer(const String& name, uint64 size, IndexSize indexSize)
		: m_Name(name),
		m_Size(size),
		m_IndexSize(indexSize)
	{

	}

	uint64 IndexBuffer::GetCount() const
	{
		switch (m_IndexSize)
		{
		case IndexSize::UInt32: return m_Size / sizeof(uint32);
		case IndexSize::UInt16: return m_Size / sizeof(uint16);
		}

		checkf(false);
		return 0;
	}

	void IndexBuffer::Resize(uint64 size)
	{
		m_Size = size;
	}
}
