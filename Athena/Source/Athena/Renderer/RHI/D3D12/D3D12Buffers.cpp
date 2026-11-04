#include "D3D12Buffers.h"
#include "Athena/Renderer/RHI/D3D12/D3D12Utils.h"


namespace Athena
{
	D3D12GPUBuffer::D3D12GPUBuffer(const GPUBufferCreateInfo& info)
		: GPUBuffer(info)
	{

	}

	D3D12GPUBuffer::~D3D12GPUBuffer()
	{
		if (m_Buffer)
		{
			D3D12RHI::Get()->DeleteFrameResourcesDeferred([buffer = m_Buffer]() mutable
				{
					buffer.Release();
				});
		}
	}

	void D3D12GPUBuffer::Init()
	{
		TRACY_PROFILE_FUNC();

		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Width = GetSize();
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.SampleDesc.Count = 1;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;

		if (GetFlags() & GBF_AllowUAV)
		{
			desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}

		m_Buffer = D3D12RHI::Get()->GetAllocator()->AllocateResource(desc, D3D12_RESOURCE_STATE_COMMON, D3D12_HEAP_TYPE_DEFAULT);
		D3D12_SET_DEBUG_NAME(m_Buffer.GetResource(), GetName());
	}
}
