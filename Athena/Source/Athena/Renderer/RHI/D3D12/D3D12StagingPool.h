#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Time.h"
#include "Athena/Renderer/RHI/D3D12/D3D12Allocator.h"


namespace Athena
{
	struct D3D12StagingBuffer
	{
		D3D12Resource Resource;
		uint64 Size = 0;
		uint64 RetireFenceValue = 0;
		Time LastTimeUsed;
	};

	class D3D12StagingPool
	{
	public:
		void Clear();

		D3D12Resource GetBuffer(uint64 size);
		void MarkAsUsed(D3D12Resource buffer);
		void Update(uint64 completedFenceValue);

	private:
		std::unordered_map<ID3D12Resource*, D3D12StagingBuffer> m_FreeBuffers;
		std::unordered_map<ID3D12Resource*, D3D12StagingBuffer> m_UsedBuffers;
	};
}
