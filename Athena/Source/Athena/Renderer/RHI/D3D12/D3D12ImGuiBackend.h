#pragma once

#include "Athena/Core/Core.h"
#include "Athena/UI/ImGuiBackend.h"
#include "Athena/Renderer/Texture.h"
#include "Athena/Renderer/RHI/D3D12/D3D12RHI.h"

#include "imgui_threaded_rendering.h"


namespace Athena
{
    // Simple free list based allocator
    struct ExampleDescriptorHeapAllocator
    {
        ID3D12DescriptorHeap* Heap = nullptr;
        D3D12_DESCRIPTOR_HEAP_TYPE  HeapType = D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
        D3D12_CPU_DESCRIPTOR_HANDLE HeapStartCpu;
        D3D12_GPU_DESCRIPTOR_HANDLE HeapStartGpu;
        UINT                        HeapHandleIncrement;
        ImVector<int>               FreeIndices;

        void Create(ID3D12Device* device, ID3D12DescriptorHeap* heap)
        {
            IM_ASSERT(Heap == nullptr && FreeIndices.empty());
            Heap = heap;
            D3D12_DESCRIPTOR_HEAP_DESC desc = heap->GetDesc();
            HeapType = desc.Type;
            HeapStartCpu = Heap->GetCPUDescriptorHandleForHeapStart();
            HeapStartGpu = Heap->GetGPUDescriptorHandleForHeapStart();
            HeapHandleIncrement = device->GetDescriptorHandleIncrementSize(HeapType);
            FreeIndices.reserve((int)desc.NumDescriptors);
            for (int n = desc.NumDescriptors; n > 0; n--)
                FreeIndices.push_back(n - 1);
        }
        void Destroy()
        {
            Heap = nullptr;
            FreeIndices.clear();
        }
        void Alloc(D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle)
        {
            IM_ASSERT(FreeIndices.Size > 0);
            int idx = FreeIndices.back();
            FreeIndices.pop_back();
            out_cpu_desc_handle->ptr = HeapStartCpu.ptr + (idx * HeapHandleIncrement);
            out_gpu_desc_handle->ptr = HeapStartGpu.ptr + (idx * HeapHandleIncrement);
        }
        void Free(D3D12_CPU_DESCRIPTOR_HANDLE out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE out_gpu_desc_handle)
        {
            int cpu_idx = (int)((out_cpu_desc_handle.ptr - HeapStartCpu.ptr) / HeapHandleIncrement);
            int gpu_idx = (int)((out_gpu_desc_handle.ptr - HeapStartGpu.ptr) / HeapHandleIncrement);
            IM_ASSERT(cpu_idx == gpu_idx);
            FreeIndices.push_back(cpu_idx);
        }
    };

	class ATHENA_API D3D12ImGuiBackend : public ImGuiBackend
	{
	public:
		D3D12ImGuiBackend();
		virtual ~D3D12ImGuiBackend();

		virtual void Init_GT(void* windowHandle) override;
		virtual void Init_RT() override;

		virtual void NewFrame_GT() override;
		virtual void NewFrame_RT() override;
		virtual void SwapDrawData() override;
		virtual void RenderDrawData() override;

		virtual void* GetTextureID(const Ref<TextureView>& texture) override;

		virtual void OnSwapChainRecreate() override;

	private:
        static inline ID3D12DescriptorHeap* m_SRVHeap;
        static inline ExampleDescriptorHeapAllocator m_SRVHeapAllocator;

        ComPtr<ID3D12DescriptorHeap> m_RTVHeap;
        std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_BackBufferRTVs;
        
		ImTextureQueue m_ImGuiTextureQueue;
		std::mutex m_TextureQueueMutex;
	};
}
