#include "D3D12Allocator.h"

#include "Athena/Renderer/RHI/D3D12/D3D12RHI.h"
#include "Athena/Renderer/RHI/D3D12/D3D12Utils.h"


namespace Athena
{
	void D3D12Resource::Release()
	{
		m_Allocation->Release();
	}

	void* D3D12Resource::MapMemory(D3D12_RANGE range)
	{
		void* mappedPtr;
		D3D12_CHECK(GetResource()->Map(0, &range, &mappedPtr));

		return mappedPtr;
	}

	void D3D12Resource::UnmapMemory()
	{
		GetResource()->Unmap(0, NULL);
	}

	void D3D12Allocator::Init()
	{
		D3D12MA::ALLOCATOR_DESC allocatorDesc = {};
		allocatorDesc.pDevice = D3D12RHI::Get()->GetDevice().Get();
		allocatorDesc.pAdapter = D3D12RHI::Get()->GetAdapter().Get();
		allocatorDesc.Flags = D3D12MA_RECOMMENDED_ALLOCATOR_FLAGS | D3D12MA::ALLOCATOR_FLAG_SINGLETHREADED;

		D3D12_CHECK(D3D12MA::CreateAllocator(&allocatorDesc, &m_Allocator));
	}

	void D3D12Allocator::Destroy()
	{
		m_Allocator->Release();
	}

	D3D12Resource D3D12Allocator::AllocateResource(const D3D12_RESOURCE_DESC& resourceDesc, D3D12_RESOURCE_STATES initialState, D3D12_HEAP_TYPE heapType, const D3D12_CLEAR_VALUE* clearValue, D3D12MA::ALLOCATION_FLAGS flags)
	{
		D3D12MA::ALLOCATION_DESC allocDesc;
		allocDesc.Flags = flags;
		allocDesc.HeapType = heapType;
		allocDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_NONE;
		allocDesc.CustomPool = NULL;
		allocDesc.pPrivateData = NULL;

		D3D12MA::Allocation* allocation;
		HRESULT hr = m_Allocator->CreateResource(
			&allocDesc,
			&resourceDesc,
			initialState,
			clearValue,
			&allocation,
			IID_NULL, NULL);

		return D3D12Resource(allocation);
	}
}
