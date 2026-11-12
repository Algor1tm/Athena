#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/RHI/RHI.h"
#include "Athena/Renderer/RHI/D3D12/D3D12Configuration.h"
#include "Athena/Renderer/RHI/D3D12/D3D12Debug.h"
#include "Athena/Renderer/RHI/D3D12/D3D12Allocator.h"
#include "Athena/Renderer/RHI/D3D12/D3D12CommandContext.h"
#include "Athena/Renderer/RHI/D3D12/D3D12StagingPool.h"

#include <d3d12.h>
#include <dxgi1_6.h>


namespace Athena
{
	struct D3DDeviceFeaturesInfo
	{
		D3D_FEATURE_LEVEL FeatureLevel;
		D3D_SHADER_MODEL ShaderModel;
		D3D_ROOT_SIGNATURE_VERSION RootSignatureVersion;

		D3D12_FEATURE_DATA_D3D12_OPTIONS3 Options3;
		D3D12_FEATURE_DATA_D3D12_OPTIONS4 Options4;
		D3D12_FEATURE_DATA_D3D12_OPTIONS5 Options5;
		D3D12_FEATURE_DATA_D3D12_OPTIONS12 Options12;
		D3D12_FEATURE_DATA_D3D12_OPTIONS18 Options18;
	};

	class ATHENA_API D3D12RHI : public RHI
	{
		// Interface functions
	public:
		D3D12RHI() = default;

		NON_COPYABLE_NON_MOVABLE(D3D12RHI);

		virtual void Init(const RendererConfig& config) override;

		virtual void Shutdown() override;

		virtual void BeginFrame(const Ref<Window>& window) override;

		virtual void EndFrame(const Ref<Window>& window) override;

		virtual Ref<CommandContext> GetContext(CommandContextType type) override;

		virtual void ForEachUniqueContext(const std::function<void(const Ref<CommandContext>&)>& func) override;

		virtual Ref<Texture> CreateTexture(const TextureCreateInfo& info) override;

		virtual Ref<TextureView> CreateTextureView(const TextureViewCreateInfo& info, Texture* texture) override;

		virtual Ref<GPUBuffer> CreateGPUBuffer(const GPUBufferCreateInfo& info) override;

		virtual Ref<ShaderCompiler> CreateShaderCompiler() override;

		virtual Ref<ShaderInstance> CreateShaderInstance(const Buffer& bytecode, const ShaderReflectionData& reflectionData) override;

		virtual Ref<SwapChain> CreateSwapChain(void* windowHandle, uint32 width, uint32 height, bool vsync) override;

		virtual Ref<ImGuiBackend> CreateImGuiBackend() override;

		virtual uint64 GetMemoryUsage() override;

		virtual const GPUCapabilities& GetGPUCapabilities() override;

		virtual const FormatSupportProperties& GetFormatSupportProperties(Format format) override;

		// D3D12 functions
	public:
		static D3D12RHI* Get()
		{
			return reinterpret_cast<D3D12RHI*>(RHI::Get());
		}

		ID3D12Device* GetDevice() const { return m_Device; }

		IDXGIFactory6* GetDXGIFactory() const { return m_DXGIFactory; }

		IDXGIAdapter4* GetAdapter() const { return m_Adapter; }

		D3D12Fence* GetFrameFence() { return &m_FrameFence; }

		D3D12Allocator* GetAllocator() { return &m_Allocator; }

		D3D12StagingPool* GetStagingPool() { return &m_StagingPool; }

		const D3DDeviceFeaturesInfo& GetFeaturesInfo() const { return m_FeaturesInfo; }

		void WaitDeviceIdle();

		template <typename FuncT>
		void DeleteFrameResourcesDeferred(FuncT&& func)
		{
			m_ResourceFreeQueues[m_FrameResourcesIndex].Enqueue(std::forward<FuncT>(func));
		}

		uint32 GetFrameResourcesIndex() const
		{
			return m_FrameResourcesIndex;
		}

	private:
		void QueryDeviceFeatures();

	private:
		ID3D12Device* m_Device;
		IDXGIFactory6* m_DXGIFactory;
		IDXGIAdapter4* m_Adapter;

		Ref<D3D12CommandContext> m_GraphicsContext;
		Ref<D3D12CommandContext> m_AsyncComputeContext;
		Ref<D3D12CommandContext> m_AsyncTransferContext;
		D3D12Fence m_FrameFence;

		D3D12Allocator m_Allocator;
		D3D12StagingPool m_StagingPool;

		uint32 m_FrameResourcesIndex = 0;
		std::vector<CommandStream> m_ResourceFreeQueues;

		D3DDeviceFeaturesInfo m_FeaturesInfo;
		GPUCapabilities m_GPUCapabilities;
	};
}

