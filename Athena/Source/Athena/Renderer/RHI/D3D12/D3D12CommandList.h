#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Time.h"
#include "Athena/Core/CommandStream.h"
#include "Athena/Renderer/RHI/D3D12/D3D12Configuration.h"

#include <d3d12.h>


namespace Athena
{
	class D3D12Fence
	{
	public:
		void Init();
		void Destroy();

		void Signal(ID3D12CommandQueue* queue);

		bool IsCompleted() const;
		uint64 GetCompletedValue() const;
		uint64 GetSignaledValue() const;
		void WaitCompletion(uint64 timeout = D3D12_DEFAULT_FENCE_TIMEOUT) const;
		void WaitValue(uint64 value, uint64 timeout = D3D12_DEFAULT_FENCE_TIMEOUT) const;

	public:
		ID3D12Fence* Fence;
		HANDLE FenceEvent;
		uint64 FenceValue = 0;
	};

	enum class CommandListState
	{
		Idle,
		RecordingCommands,
		Submitted,
	};

	class D3D12CommandList
	{
	public:
		D3D12CommandList(D3D12_COMMAND_LIST_TYPE type);
		~D3D12CommandList();

		template <typename FuncT>
		void AddCallbackOnCompletion(FuncT&& func)
		{
			OnCompletionCallbacks.Enqueue(std::forward<FuncT>(func));
		}

		void Begin();
		void End();

		void OnCompletion();
		void WaitCompletion();
		void Execute(ID3D12CommandQueue* queue);

	public:
		CommandListState State;
		ID3D12CommandAllocator* Allocator;
		ID3D12GraphicsCommandList* CmdList;
		CommandStream OnCompletionCallbacks;
		D3D12Fence Fence;
		Time LastUsedTime;
	};
}
