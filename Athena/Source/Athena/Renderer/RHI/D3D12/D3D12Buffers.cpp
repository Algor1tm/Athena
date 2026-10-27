#include "D3D12Buffers.h"
#include "Athena/Renderer/RHI/D3D12/D3D12Utils.h"


namespace Athena
{
	D3D12VertexBuffer::D3D12VertexBuffer(const String& name, uint64 size)
		: VertexBuffer(name, size)
	{

	}

	D3D12VertexBuffer::~D3D12VertexBuffer()
	{
		CleanUp();
	}

	void D3D12VertexBuffer::Init()
	{
		TRACY_PROFILE_FUNC();

		Resize(GetSize());
	}

	void D3D12VertexBuffer::Upload(const Buffer& buffer, uint64 offset)
	{
		TRACY_PROFILE_FUNC();

		check(GetSize() >= buffer.Size() + offset && buffer.Data() != nullptr, "Invalid initialization buffer for VertexBuffer!");

		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Width = buffer.Size();
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.SampleDesc.Count = 1;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;

		D3D12Resource stagingBuffer = D3D12RHI::Get()->GetAllocator()->AllocateResource(desc, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_HEAP_TYPE_UPLOAD);

		void* mappedData = stagingBuffer.MapMemory({ 0, buffer.Size() });
		memcpy(mappedData, buffer.Data(), buffer.Size());
		stagingBuffer.UnmapMemory();

		D3D12CommandList* list = D3D12RHI::Get()->GetCmdListManager()->GetTransferCommandList();

		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = m_Buffer.GetResource();
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;

		list->CommandList->ResourceBarrier(1, &barrier);
		list->CommandList->CopyBufferRegion(m_Buffer.GetResource(), offset, stagingBuffer.GetResource(), 0, buffer.Size());

		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

		list->CommandList->ResourceBarrier(1, &barrier);

		list->AddCallbackOnCompletion([stagingBuffer]() mutable
			{
				stagingBuffer.Release();
			});
	}

	void D3D12VertexBuffer::Resize(uint64 size)
	{
		TRACY_PROFILE_FUNC();

		VertexBuffer::Resize(size);

		CleanUp();

		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Width = size;
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.SampleDesc.Count = 1;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;

		m_Buffer = D3D12RHI::Get()->GetAllocator()->AllocateResource(desc, D3D12_RESOURCE_STATE_COMMON, D3D12_HEAP_TYPE_DEFAULT);
		D3D12_SET_DEBUG_NAME(m_Buffer.GetResource(), GetName());
	}

	void D3D12VertexBuffer::CleanUp()
	{
		if (m_Buffer)
		{
			D3D12RHI::Get()->DeleteFrameResourcesDeferred([buffer = m_Buffer, name = GetName()]() mutable
				{
					buffer.Release();
				});
		}
	}


	D3D12IndexBuffer::D3D12IndexBuffer(const String& name, uint64 size, IndexSize indexSize)
		: IndexBuffer(name, size, indexSize)
	{

	}

	D3D12IndexBuffer::~D3D12IndexBuffer()
	{
		CleanUp();
	}

	void D3D12IndexBuffer::Init()
	{
		TRACY_PROFILE_FUNC();

		Resize(GetSize());
	}

	void D3D12IndexBuffer::Upload(const Buffer& buffer, uint64 offset)
	{
		TRACY_PROFILE_FUNC();

		check(GetSize() >= buffer.Size() + offset && buffer.Data() != nullptr, "Invalid initialization buffer for VertexBuffer!");

		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Width = buffer.Size();
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.SampleDesc.Count = 1;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;

		D3D12Resource stagingBuffer = D3D12RHI::Get()->GetAllocator()->AllocateResource(desc, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_HEAP_TYPE_UPLOAD);

		void* mappedData = stagingBuffer.MapMemory({ 0, buffer.Size() });
		memcpy(mappedData, buffer.Data(), buffer.Size());
		stagingBuffer.UnmapMemory();

		D3D12CommandList* list = D3D12RHI::Get()->GetCmdListManager()->GetTransferCommandList();

		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = m_Buffer.GetResource();
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;

		list->CommandList->ResourceBarrier(1, &barrier);

		list->CommandList->CopyBufferRegion(m_Buffer.GetResource(), offset, stagingBuffer.GetResource(), 0, buffer.Size());

		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_INDEX_BUFFER;

		list->CommandList->ResourceBarrier(1, &barrier);

		list->AddCallbackOnCompletion([stagingBuffer]() mutable
			{
				stagingBuffer.Release();
			});
	}

	void D3D12IndexBuffer::Resize(uint64 size)
	{
		TRACY_PROFILE_FUNC();

		IndexBuffer::Resize(size);

		CleanUp();

		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Width = size;
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.SampleDesc.Count = 1;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;

		m_Buffer = D3D12RHI::Get()->GetAllocator()->AllocateResource(desc, D3D12_RESOURCE_STATE_COMMON, D3D12_HEAP_TYPE_DEFAULT);
		D3D12_SET_DEBUG_NAME(m_Buffer.GetResource(), GetName());
	}

	void D3D12IndexBuffer::CleanUp()
	{
		if (m_Buffer)
		{
			D3D12RHI::Get()->DeleteFrameResourcesDeferred([buffer = m_Buffer, name = GetName()]() mutable
				{
					buffer.Release();
				});
		}
	}
}
