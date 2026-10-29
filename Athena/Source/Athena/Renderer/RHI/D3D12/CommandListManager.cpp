#include "CommandListManager.h"

#include "Athena/Renderer/RHI/D3D12/D3D12RHI.h"
#include "Athena/Renderer/RHI/D3D12/D3D12Utils.h"

#define COMMAND_BUFFER_DELETE_DELAY 1 // seconds


namespace Athena
{
	D3D12CommandList::D3D12CommandList()
		: State(CommandListState::Idle),
		FenceValue(0),
		OnCompletionCallbacks(10 * 1024), // 10Kb
		LastUsedTime(0)
	{
		D3D12_CHECK(D3D12RHI::Get()->GetDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&Allocator)));
		D3D12_CHECK(D3D12RHI::Get()->GetDevice()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, Allocator.Get(), nullptr, IID_PPV_ARGS(&CommandList)));
		D3D12_CHECK(CommandList->Close());

		D3D12_CHECK(D3D12RHI::Get()->GetDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence)));
		FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	}

	D3D12CommandList::~D3D12CommandList()
	{
		CloseHandle(FenceEvent);
	}

	void D3D12CommandList::Begin()
	{
		ensuref(State == CommandListState::Idle);

		D3D12_CHECK(Allocator->Reset());
		D3D12_CHECK(CommandList->Reset(Allocator.Get(), nullptr));

		State = CommandListState::RecordingCommands;
	}

	void D3D12CommandList::End()
	{
		ensuref(State == CommandListState::RecordingCommands);

		D3D12_CHECK(CommandList->Close());
	}

	void D3D12CommandList::OnCompletion()
	{
		TRACY_PROFILE_FUNC();

		OnCompletionCallbacks.Flush();
		State = CommandListState::Idle;
		LastUsedTime = Time::GetSystemCurrentTime();
	}

	void CommandListManager::Init()
	{
		for (uint32 i = 0; i < D3D12::GetMaxFramesInFlight(); ++i)
		{
			D3D12CommandList* list = new D3D12CommandList();
			m_PresentCommandLists.push_back(list);

			D3D12_SET_DEBUG_NAME(list->CommandList, std::format("CmdListForPresent_{}", i));
			D3D12_SET_DEBUG_NAME(list->Fence, std::format("CmdListForPresentFence_{}", i));
		}
	}

	void CommandListManager::Destroy()
	{
		for (uint32 i = 0; i < D3D12::GetMaxFramesInFlight(); ++i)
		{
			D3D12CommandList* list = m_PresentCommandLists[i];

			WaitForCmdList(list);
			delete list;
		}

		for (uint32 i = 0; i < m_CommandLists.size(); ++i)
		{
			D3D12CommandList* list = m_CommandLists[i];

			WaitForCmdList(list);
			delete list;
		}

		m_CommandLists.clear();
	}

	D3D12CommandList* CommandListManager::GetCommandList()
	{
		D3D12CommandList* list = FindFreeCommandList();
		D3D12_SET_DEBUG_NAME(list->CommandList, "DirectCommandList");
		D3D12_SET_DEBUG_NAME(list->Fence, "DirectCommandListFence");

		list->Begin();

		return list;
	}

	D3D12CommandList* CommandListManager::GetTransferCommandList()
	{
		if (!m_TransferCommandList)
		{
			m_TransferCommandList = FindFreeCommandList();
			D3D12_SET_DEBUG_NAME(m_TransferCommandList->CommandList, "TransferCommandList");
			D3D12_SET_DEBUG_NAME(m_TransferCommandList->Fence, "TransferCommandListFence");

			m_TransferCommandList->Begin();
		}

		return m_TransferCommandList;
	}

	D3D12CommandList* CommandListManager::GetPresentCommandList()
	{
		D3D12CommandList* list = m_PresentCommandLists[D3D12RHI::Get()->GetFrameResourcesIndex()];
		if (list->State == CommandListState::Idle)
		{
			list->Begin();
		}

		return list;
	}

	void CommandListManager::WaitForCmdList(D3D12CommandList* commandList)
	{
		TRACY_PROFILE_FUNC();

		if (commandList->State == CommandListState::Submitted)
		{
			if (commandList->Fence->GetCompletedValue() < commandList->FenceValue)
			{
				commandList->Fence->SetEventOnCompletion(commandList->FenceValue, commandList->FenceEvent);
				WaitForSingleObject(commandList->FenceEvent, INFINITE);
			}

			commandList->OnCompletion();
		}
	}

	void CommandListManager::Submit(D3D12CommandList* commandList)
	{
		TRACY_PROFILE_FUNC();

		ensuref(m_TransferCommandList != commandList && commandList != nullptr);
		ensuref(commandList->State == CommandListState::RecordingCommands);

		if (m_TransferCommandList)
		{
			SubmitTransfer();
		}

		commandList->End();

		ID3D12CommandList* commandLists[] = { commandList->CommandList.Get() };

		D3D12RHI::Get()->GetQueue()->ExecuteCommandLists(1, commandLists);

		commandList->FenceValue++;
		D3D12_CHECK(D3D12RHI::Get()->GetQueue()->Signal(commandList->Fence.Get(), commandList->FenceValue));

		commandList->State = CommandListState::Submitted;
	}

	void CommandListManager::SubmitTransfer()
	{
		TRACY_PROFILE_FUNC();

		if (!m_TransferCommandList)
			return;

		D3D12CommandList* uploadList = m_TransferCommandList;
		m_TransferCommandList = nullptr;

		Submit(uploadList);
	}

	void CommandListManager::SubmitForPresent(D3D12CommandList* commandList)
	{
		TRACY_PROFILE_FUNC();

		ensuref(m_TransferCommandList != commandList && commandList != nullptr);
		ensuref(commandList->State == CommandListState::RecordingCommands);

		if (m_TransferCommandList)
		{
			SubmitTransfer();
		}

		commandList->End();

		ID3D12CommandList* commandLists[] = { commandList->CommandList.Get() };

		D3D12RHI::Get()->GetQueue()->ExecuteCommandLists(1, commandLists);
	}

	void CommandListManager::SignalPresent(D3D12CommandList* commandList)
	{
		commandList->FenceValue++;
		D3D12_CHECK(D3D12RHI::Get()->GetQueue()->Signal(commandList->Fence.Get(), commandList->FenceValue));

		commandList->State = CommandListState::Submitted;
	}

	void CommandListManager::NewFrame()
	{
		TRACY_PROFILE_FUNC();

		std::vector<uint32> buffersToDelete;

		for (uint32 i = 0; i < m_CommandLists.size(); ++i)
		{
			D3D12CommandList* list = m_CommandLists[i];

			if (list->State == CommandListState::Idle)
			{
				if (Time::GetSystemCurrentTime() - list->LastUsedTime >= Time::Seconds(COMMAND_BUFFER_DELETE_DELAY))
				{
					buffersToDelete.push_back(i);
				}
			}
			else if (list->State == CommandListState::Submitted)
			{
				if (list->Fence->GetCompletedValue() >= list->FenceValue)
				{
					list->OnCompletion();
				}
			}
		}

		for (uint32 index : buffersToDelete)
		{
			D3D12CommandList* list = m_CommandLists[index];
			m_CommandLists.erase(m_CommandLists.begin() + index);
			delete list;
		}
	}

	void CommandListManager::WaitDeviceIdle()
	{
		for (uint32 i = 0; i < m_CommandLists.size(); ++i)
		{
			D3D12CommandList* list = m_CommandLists[i];

			if (list->State == CommandListState::Submitted)
			{
				WaitForCmdList(list);
			}
		}

		for (uint32 i = 0; i < m_PresentCommandLists.size(); ++i)
		{
			D3D12CommandList* list = m_PresentCommandLists[i];

			if (list->State == CommandListState::Submitted)
			{
				WaitForCmdList(list);
			}
		}
	}

	D3D12CommandList* CommandListManager::FindFreeCommandList()
	{
		TRACY_PROFILE_FUNC();

		for (uint32 i = 0; i < m_CommandLists.size(); ++i)
		{
			D3D12CommandList* buffer = m_CommandLists[i];
			if (buffer->State == CommandListState::Idle)
			{
				return buffer;
			}
		}

		return AllocateCommandList();
	}

	D3D12CommandList* CommandListManager::AllocateCommandList()
	{
		TRACY_PROFILE_FUNC();

		D3D12CommandList* list = new D3D12CommandList();
		m_CommandLists.push_back(list);

		return list;
	}
}
