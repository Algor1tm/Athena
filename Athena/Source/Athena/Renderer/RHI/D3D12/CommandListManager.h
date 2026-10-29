#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Time.h"
#include "Athena/Core/CommandStream.h"

#include <d3d12.h>
#include <wrl/client.h>


namespace Athena
{
	using Microsoft::WRL::ComPtr;

	enum class CommandListState
	{
		Idle,
		RecordingCommands,
		Submitted,
	};

	class D3D12CommandList
	{
	public:
		D3D12CommandList();
		~D3D12CommandList();

		template <typename FuncT>
		void AddCallbackOnCompletion(FuncT&& func)
		{
			OnCompletionCallbacks.Submit(std::forward<FuncT>(func));
		}

		void Begin();
		void End();

		void OnCompletion();

	public:
		CommandListState State;
		ComPtr<ID3D12CommandAllocator> Allocator;
		ComPtr<ID3D12GraphicsCommandList> CommandList;
		CommandStream OnCompletionCallbacks;
		uint64 FenceValue;
		ComPtr<ID3D12Fence> Fence;
		HANDLE FenceEvent;
		Time LastUsedTime;
	};

	class CommandListManager
	{
	public:
		void Init();
		void Destroy();

		D3D12CommandList* GetCommandList();
		D3D12CommandList* GetTransferCommandList();
		D3D12CommandList* GetPresentCommandList();

		void WaitForCmdList(D3D12CommandList* commandList);

		void Submit(D3D12CommandList* commandList);
		void SubmitTransfer();

		void SubmitForPresent(D3D12CommandList* commandList);
		void SignalPresent(D3D12CommandList* commandList);

		void NewFrame();

		void WaitDeviceIdle();

	private:
		D3D12CommandList* FindFreeCommandList();
		D3D12CommandList* AllocateCommandList();

	private:
		std::vector<D3D12CommandList*> m_CommandLists;
		D3D12CommandList* m_TransferCommandList = nullptr;

		std::vector<D3D12CommandList*> m_PresentCommandLists;
	};
}
