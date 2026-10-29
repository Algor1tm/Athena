#include "D3D12ImGuiBackend.h"

#include "Athena/Core/Application.h"
#include "Athena/Renderer/RHI/D3D12/D3D12Utils.h"
#include "Athena/Renderer/RHI/D3D12/D3D12SwapChain.h"

#include <ImGui/backends/imgui_impl_glfw.h>
#include <ImGui/backends/imgui_impl_dx12.h>


namespace Athena
{
	D3D12ImGuiBackend::D3D12ImGuiBackend()
	{

	}

	D3D12ImGuiBackend::~D3D12ImGuiBackend()
	{
		D3D12RHI::Get()->DeleteFrameResourcesDeferred([textureQueue = m_ImGuiTextureQueue]() mutable
			{
				textureQueue.Shutdown();

				ImGui_ImplDX12_Shutdown();
				ImGui_ImplGlfw_Shutdown();

				m_SRVHeapAllocator.Destroy();
				m_SRVHeap->Release();

				ImGui::DestroyContext();
			});
	}

	void D3D12ImGuiBackend::Init_GT(void* windowHandle)
	{
		m_ImGuiTextureQueue.InFlightFrames = D3D12::GetMaxFramesInFlight();
		m_ImGuiTextureQueue.UpdateTexFunc = ImGui_ImplDX12_UpdateTexture;

		ImGui_ImplGlfw_InitForOther(reinterpret_cast<GLFWwindow*>(windowHandle), true);
		ImGui_ImplDX12_Init_GT();
	}

	void D3D12ImGuiBackend::Init_RT()
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		desc.NumDescriptors = 64;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

		D3D12_CHECK(D3D12RHI::Get()->GetDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_SRVHeap)));
		m_SRVHeapAllocator.Create(D3D12RHI::Get()->GetDevice().Get(), m_SRVHeap);

		ImGui_ImplDX12_InitInfo init_info = {};
		init_info.Device = D3D12RHI::Get()->GetDevice().Get();
		init_info.CommandQueue = D3D12RHI::Get()->GetQueue().Get();
		init_info.NumFramesInFlight = D3D12::GetMaxFramesInFlight();
		init_info.RTVFormat = Application::Get().GetWindow().GetSwapChain().As<D3D12SwapChain>()->GetFormat();
		init_info.DSVFormat = DXGI_FORMAT_UNKNOWN;
		init_info.SrvDescriptorHeap = m_SRVHeap;
		init_info.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle) { return m_SRVHeapAllocator.Alloc(out_cpu_handle, out_gpu_handle); };
		init_info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle) { return m_SRVHeapAllocator.Free(cpu_handle, gpu_handle); };
		ImGui_ImplDX12_Init(&init_info);

		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		heapDesc.NumDescriptors = D3D12::GetMaxFramesInFlight();
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		heapDesc.NodeMask = 1;

		D3D12_CHECK(D3D12RHI::Get()->GetDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_RTVHeap)));

		SIZE_T rtvDescriptorSize = D3D12RHI::Get()->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_RTVHeap->GetCPUDescriptorHandleForHeapStart();

		for (UINT i = 0; i < D3D12::GetMaxFramesInFlight(); i++)
		{
			m_BackBufferRTVs.push_back(rtvHandle);
			rtvHandle.ptr += rtvDescriptorSize;
		}

		OnSwapChainRecreate();
	}

	void D3D12ImGuiBackend::NewFrame_GT()
	{
		std::unique_lock lock(m_TextureQueueMutex);
		m_ImGuiTextureQueue.PreNewFrame();
	}

	void D3D12ImGuiBackend::NewFrame_RT()
	{
		TRACY_PROFILE_FUNC();

		ImGui_ImplDX12_NewFrame();

		ImGuiContext& g = *GImGui;
		for (ImGuiViewportP* viewport : g.Viewports)
		{
			viewport->SnapshotIndex_RT = (viewport->SnapshotIndex_RT + 1) % 2;
		}
	}

	void D3D12ImGuiBackend::SwapDrawData()
	{
		TRACY_PROFILE_FUNC();

		ImGuiContext& g = *GImGui;

		{
			// Get draw data from main viewport (index 0), typically texture lists are shared between viewports draw data
			ImGuiViewportP* viewport = g.Viewports[0];

			std::unique_lock lock(m_TextureQueueMutex);
			m_ImGuiTextureQueue.QueueRequests(&viewport->DrawDataP);
		}

		// Snapshot draw data for render thread
		for (ImGuiViewportP* viewport : g.Viewports)
		{
			viewport->SnapshotIndex_GT = (viewport->SnapshotIndex_GT + 1) % 2;
			viewport->Snapshots[viewport->SnapshotIndex_GT].DrawDataSnapshot.SnapUsingSwap(&viewport->DrawDataP, ImGui::GetTime());
			viewport->Snapshots[viewport->SnapshotIndex_GT].Size = viewport->Size;
			viewport->Snapshots[viewport->SnapshotIndex_GT].Flags = viewport->Flags;
		}
	}

	void D3D12ImGuiBackend::RenderDrawData()
	{
		TRACY_PROFILE_FUNC();

		if (!Application::Get().GetWindow().GetSwapChain())
			return;

		Ref<D3D12SwapChain> swapChain = Application::Get().GetWindow().GetSwapChain().As<D3D12SwapChain>();
		D3D12CommandList* commandList = D3D12RHI::Get()->GetCmdListManager()->GetPresentCommandList();

		uint32 width = swapChain->GetWidth();
		uint32 height = swapChain->GetHeight();

		uint32 backBufferIndex = swapChain->GetCurrentBackBufferIndex();
		D3D12CommandList* list = D3D12RHI::Get()->GetCmdListManager()->GetPresentCommandList();

		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = swapChain->GetBackBuffer(backBufferIndex);
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		list->CommandList->ResourceBarrier(1, &barrier);

		const float clear_color[4] = { 0.f, 0.f, 0.f, 1.f };
		list->CommandList->ClearRenderTargetView(m_BackBufferRTVs[backBufferIndex], clear_color, 0, nullptr);
		list->CommandList->OMSetRenderTargets(1, &m_BackBufferRTVs[backBufferIndex], FALSE, nullptr);
		list->CommandList->SetDescriptorHeaps(1, &m_SRVHeap);

		ImDrawData* drawData = ImGui::GetDrawData();
		{
			std::unique_lock lock(m_TextureQueueMutex);
			m_ImGuiTextureQueue.ProcessRequests(drawData);
		}

		ImGui_ImplDX12_RenderDrawData(drawData, list->CommandList.Get());

		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		list->CommandList->ResourceBarrier(1, &barrier);

		D3D12RHI::Get()->GetCmdListManager()->SubmitForPresent(list);
	}

	void* D3D12ImGuiBackend::GetTextureID(const Ref<TextureView>& texture)
	{
		return NULL;
	}

	void D3D12ImGuiBackend::OnSwapChainRecreate()
	{
		if (m_BackBufferRTVs.empty())
			return;

		Ref<D3D12SwapChain> swapChain = Application::Get().GetWindow().GetSwapChain().As<D3D12SwapChain>();

		for (UINT i = 0; i < D3D12::GetMaxFramesInFlight(); i++)
		{
			D3D12RHI::Get()->GetDevice()->CreateRenderTargetView(swapChain->GetBackBuffer(i), nullptr, m_BackBufferRTVs[i]);
		}
	}
}
