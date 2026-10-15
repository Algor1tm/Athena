#include "GPUBuffers.h"
#include "Athena/Renderer/RenderThread.h"
#include "Athena/Renderer/RHI/RHI.h"


namespace Athena
{
	void VertexMemoryLayout::CalculateOffsetsAndStride()
	{
		auto& formatProperties = RHI::Get()->GetGPUCapabilities().Formats;

		uint32 offset = 0;
		for (auto& elem : m_Elements)
		{
			ensure(formatProperties[elem.ElementFormat].IsVertexBufferSupported, "Format is not supported for vertex buffers!");
			uint32 elemSize = formatProperties[elem.ElementFormat].BytesPerPixel;

			elem.Offset = offset;
			offset += elemSize;
			m_Stride += elemSize;
		}
	}

	Ref<VertexBuffer> VertexBuffer::Create(const String& name, uint64 size, Buffer* vertices)
	{
		Ref<VertexBuffer> vertexBuffer = RHI::Get()->CreateVertexBuffer();

		vertexBuffer->m_Name = name;
		vertexBuffer->m_Size = size;

		if (vertices)
		{
			Buffer verticesCopy = Buffer::Copy(*vertices);
			check(vertexBuffer->m_Size >= verticesCopy.Size() && verticesCopy.Data() != nullptr, "Invalid initialization buffer for VertexBuffer!");

			ENQUEUE_RENDER_COMMAND_ANY_THREAD([vertexBuffer, verticesCopyRT = std::move(verticesCopy)]()
				{
					vertexBuffer->Init();
					vertexBuffer->UploadData(verticesCopyRT);
				});
		}
		else
		{
			ENQUEUE_RENDER_COMMAND_ANY_THREAD([vertexBuffer]()
				{
					vertexBuffer->Init();
				});
		}

		return vertexBuffer;
	}

	void VertexBuffer::Resize(uint64 size)
	{
		m_Size = size;
	}

	Ref<IndexBuffer> IndexBuffer::Create(const String& name, uint64 size, Buffer* indices, IndexSize indexSize)
	{
		Ref<IndexBuffer> indexBuffer = RHI::Get()->CreateIndexBuffer();

		indexBuffer->m_Name = name;
		indexBuffer->m_Size = size;
		indexBuffer->m_IndexSize = indexSize;

		if (indices)
		{
			Buffer indicesCopy = Buffer::Copy(*indices);
			check(indexBuffer->m_Size >= indicesCopy.Size() && indicesCopy.Data() != nullptr, "Invalid initialization buffer for IndexBuffer!");

			ENQUEUE_RENDER_COMMAND_ANY_THREAD([indexBuffer, indicesCopyRT = std::move(indicesCopy)]()
				{
					indexBuffer->Init();
					indexBuffer->UploadData(indicesCopyRT);
				});
		}
		else
		{
			ENQUEUE_RENDER_COMMAND_ANY_THREAD([indexBuffer]()
				{
					indexBuffer->Init();
				});
		}

		return indexBuffer;
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
