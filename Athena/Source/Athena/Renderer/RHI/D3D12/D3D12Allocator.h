#pragma once

#include "Athena/Core/Core.h"

#include <D3D12MemoryAllocator/D3D12MemAlloc.h>
#include <d3d12.h>


namespace Athena
{
	class D3D12Resource
	{
	public:
		D3D12Resource() = default;

		D3D12Resource(D3D12MA::Allocation* alloc)
			: m_Allocation(alloc)
		{
		}

		void Release();

		void* MapMemory(D3D12_RANGE range = { 0 , 0 });
		void UnmapMemory();

		ID3D12Resource* GetResource() const { return m_Allocation->GetResource(); }
		D3D12MA::Allocation* GetAllocation() const { return m_Allocation; }
		
		operator bool() const { return !(m_Allocation == nullptr); }

	private:
		D3D12MA::Allocation* m_Allocation;
	};

	class D3D12Allocator
	{
	public:
		void Init();
		void Destroy();

		D3D12Resource AllocateResource(const D3D12_RESOURCE_DESC& resourceDesc, D3D12_RESOURCE_STATES initialState, D3D12_HEAP_TYPE heapType,
			const D3D12_CLEAR_VALUE* clearValue = nullptr, D3D12MA::ALLOCATION_FLAGS flags = D3D12MA::ALLOCATION_FLAG_NONE);

	private:
		D3D12MA::Allocator* m_Allocator = nullptr;
	};
}
