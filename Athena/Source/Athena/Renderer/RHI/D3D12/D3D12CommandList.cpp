#include "D3D12CommandList.h"

#include "Athena/Renderer/RHI/D3D12/D3D12RHI.h"
#include "Athena/Renderer/RHI/D3D12/D3D12Utils.h"


namespace Athena
{
	void D3D12Fence::Init()
	{
		FenceValue = 0;
		D3D12_CHECK(D3D12RHI::Get()->GetDevice()->CreateFence(FenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence)));
		FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	}

	void D3D12Fence::Destroy()
	{
		CloseHandle(FenceEvent);
		Fence->Release();
	}

	void D3D12Fence::Signal(ID3D12CommandQueue* queue)
	{
		FenceValue++;
		D3D12_CHECK(queue->Signal(Fence, FenceValue));
	}

	bool D3D12Fence::IsCompleted() const
	{
		return Fence->GetCompletedValue() == FenceValue;
	}

	uint64 D3D12Fence::GetCompletedValue() const
	{
		return Fence->GetCompletedValue();
	}

	uint64 D3D12Fence::GetSignaledValue() const
	{
		return FenceValue;
	}

	void D3D12Fence::WaitCompletion(uint64 timeout) const
	{
		WaitValue(FenceValue, timeout);
	}

	void D3D12Fence::WaitValue(uint64 value, uint64 timeout) const
	{
		TRACY_PROFILE_FUNC();

		if (!IsCompleted())
		{
			D3D12_CHECK(Fence->SetEventOnCompletion(value, FenceEvent));
			WaitForSingleObject(FenceEvent, timeout);
		}
	}


	D3D12CommandList::D3D12CommandList(D3D12_COMMAND_LIST_TYPE type)
		: State(CommandListState::Idle),
		LastUsedTime(0)
	{
		D3D12_CHECK(D3D12RHI::Get()->GetDevice()->CreateCommandAllocator(type, IID_PPV_ARGS(&Allocator)));
		D3D12_CHECK(D3D12RHI::Get()->GetDevice()->CreateCommandList(0, type, Allocator, nullptr, IID_PPV_ARGS(&CmdList)));
		D3D12_CHECK(CmdList->Close());

		Fence.Init();
	}

	D3D12CommandList::~D3D12CommandList()
	{
		ensuref(State == CommandListState::Idle);

		CmdList->Release();
		Allocator->Release();
		Fence.Destroy();
	}

	void D3D12CommandList::Begin()
	{
		ensuref(State == CommandListState::Idle);

		D3D12_CHECK(Allocator->Reset());
		D3D12_CHECK(CmdList->Reset(Allocator, nullptr));

		State = CommandListState::RecordingCommands;
	}

	void D3D12CommandList::End()
	{
		ensuref(State == CommandListState::RecordingCommands);

		D3D12_CHECK(CmdList->Close());
	}

	void D3D12CommandList::OnCompletion()
	{
		TRACY_PROFILE_FUNC();

		OnCompletionCallbacks.Flush();
		State = CommandListState::Idle;
		LastUsedTime = Time::GetSystemCurrentTime();
	}

	void D3D12CommandList::WaitCompletion()
	{
		TRACY_PROFILE_FUNC();

		if (State == CommandListState::Submitted)
		{
			Fence.WaitCompletion();

			OnCompletion();
		}
	}

	void D3D12CommandList::Execute(ID3D12CommandQueue* queue)
	{
		ID3D12CommandList* commandLists[] = { CmdList };
		queue->ExecuteCommandLists(1, commandLists);

		Fence.Signal(queue);
		State = CommandListState::Submitted;
	}
}
