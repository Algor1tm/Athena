#include "D3D12RHI.h"

#include "Athena/Core/Application.h"
#include "Athena/Core/ConsoleManager.h"
#include "Athena/Utils/StringUtils.h"

#include "Athena/Renderer/RHI/D3D12/D3D12Utils.h"
#include "Athena/Renderer/RHI/D3D12/D3D12Buffers.h"
#include "Athena/Renderer/RHI/D3D12/D3D12CommandList.h"
#include "Athena/Renderer/RHI/D3D12/D3D12ImGuiBackend.h"
#include "Athena/Renderer/RHI/D3D12/D3D12SwapChain.h"
#include "Athena/Renderer/RHI/D3D12/D3D12Texture.h"
#include "Athena/Renderer/RHI/D3D12/D3D12TracyZone.h"


namespace Athena
{
#if D3D12_ENABLE_DEBUG_INFO
	static AutoCVar<int32> CVarD3D12ValidationLevel(
		"D3D12.ValidationLevel",
		3,
		"Sets debug report level 0 - 5 (0 - disable validation, default - 3)."
	);

	static AutoCVar<bool> CVarD3D12EnableGPUValidation(
		"D3D12.GPUValidation",
		false,
		"Specifices if gpu validation will be enabled or not (D3D12.ValidationLevel should be >0)."
	);
#endif

	static const char* GetVendorName(uint32 vendorID)
	{
		if (vendorID == 0x10DE)
			return "NVIDIA";
		else if (vendorID == 0x1002)
			return "AMD";
		else if (vendorID == 0x8086)
			return "Intel";
		else if (vendorID == 0x1414)
			return "WARP";
		else if (vendorID == 0x13B5)
			return "ARM (Mali)";
		else if (vendorID == 0x5143)
			return "Qualcomm (Adreno)";
		else if (vendorID == 0x1010)
			return "Imagination Technologies";

		return "Unknown";
	}

	void D3D12RHI::Init(const RendererConfig& config)
	{
		bool enableValidation = false;

#if D3D12_ENABLE_DEBUG_INFO
		enableValidation = CVarD3D12ValidationLevel.GetInt() > 0;
#endif

		if (enableValidation)
		{
			ComPtr<ID3D12Debug> debugLayer;

			if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer))))
			{
				debugLayer->EnableDebugLayer();

				ComPtr<ID3D12Debug1> debugLayer1;
				if (CVarD3D12EnableGPUValidation.GetBool() && SUCCEEDED(debugLayer.As(&debugLayer1)))
				{
					debugLayer1->SetEnableGPUBasedValidation(true);
				}
			}

			ATN_LOG_INFO(D3D12, "D3D12 debug layer is enabled!");
		}

		UINT createFactoryFlags = enableValidation ? DXGI_CREATE_FACTORY_DEBUG : 0;
		D3D12_CHECK(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&m_DXGIFactory)));

		ComPtr<IDXGIAdapter1> adapter;

		String message = "Available GPUs: \n\t";
		for (UINT adapterIndex = 0; m_DXGIFactory->EnumAdapterByGpuPreference(adapterIndex, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND; ++adapterIndex)
		{
			// Select first one
			if (adapterIndex == 0)
			{
				adapter.As(&m_Adapter);
			}

			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);

			message += fmt::format("{}\n\t", ToString(desc.Description));
		}

		ATN_LOG_INFO(Vulkan, message);
		message = fmt::format("Selected GPU description: \n\t");

		DXGI_ADAPTER_DESC1 desc;
		m_Adapter->GetDesc1(&desc);

		const char* vendorName = GetVendorName(desc.VendorId);

		message += fmt::format("Description: {} \n\t", ToString(desc.Description));
		message += fmt::format("VendorName: {} \n\t", vendorName);
		message += fmt::format("DeviceId: {} \n\t", desc.DeviceId);
		message += fmt::format("SubSysId: {} \n\t", desc.SubSysId);
		message += fmt::format("Revision: {} \n\t", desc.Revision);
		message += fmt::format("DedicatedVideoMemory: {} \n\t", Utils::MemoryBytesToString(desc.DedicatedVideoMemory));
		message += fmt::format("DedicatedSystemMemory: {} \n\t", Utils::MemoryBytesToString(desc.DedicatedSystemMemory));
		message += fmt::format("SharedSystemMemory: {} \n\t", Utils::MemoryBytesToString(desc.SharedSystemMemory));
		message += fmt::format("Is Software: {} \n\t", desc.Flags == DXGI_ADAPTER_FLAG_SOFTWARE);

		ATN_LOG_INFO(Vulkan, message);

		D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_12_0;
		if (FAILED(D3D12CreateDevice(m_Adapter.Get(), featureLevel, IID_PPV_ARGS(&m_Device))))
		{
			featureLevel = D3D_FEATURE_LEVEL_11_0;
			HRESULT hresult = D3D12CreateDevice(m_Adapter.Get(), featureLevel, IID_PPV_ARGS(&m_Device));

			if (FAILED(hresult))
			{
				ATN_LOG_FATAL(D3D12, "Failed to create D3D12 device!");
			}
		}

#if D3D12_ENABLE_DEBUG_INFO
		ComPtr<ID3D12InfoQueue1> infoQueue;
		if (enableValidation && SUCCEEDED(m_Device->QueryInterface(IID_PPV_ARGS(&infoQueue))))
		{
			int severityLevel = CVarD3D12ValidationLevel.GetInt();
			std::vector<D3D12_MESSAGE_SEVERITY> severityList;

			switch (severityLevel)
			{
			case 5: severityList.push_back(D3D12_MESSAGE_SEVERITY_MESSAGE);
			case 4: severityList.push_back(D3D12_MESSAGE_SEVERITY_INFO);
			case 3: severityList.push_back(D3D12_MESSAGE_SEVERITY_WARNING);
			case 2: severityList.push_back(D3D12_MESSAGE_SEVERITY_ERROR);
			case 1: severityList.push_back(D3D12_MESSAGE_SEVERITY_CORRUPTION);
			}

			D3D12_INFO_QUEUE_FILTER filter = {};
			filter.AllowList.pSeverityList = severityList.data();
			filter.AllowList.NumSeverities = severityList.size();

			D3D12_CHECK(infoQueue->AddStorageFilterEntries(&filter));

			DWORD callbackCookie;
			D3D12_CHECK(infoQueue->RegisterMessageCallback(D3D12::DebugCallback, D3D12_MESSAGE_CALLBACK_FLAG_NONE, nullptr, &callbackCookie));

			ATN_LOG_INFO(D3D12, "D3D12 debug device is enabled!");
		}
#endif

		QueryDeviceFeatures();

		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.NodeMask = 0;

		D3D12_CHECK(m_Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_Queue)));

		m_ResourceFreeQueues.resize(D3D12::GetMaxFramesInFlight());
		for (uint32 i = 0; i < m_ResourceFreeQueues.size(); ++i)
		{
			m_ResourceFreeQueues[i] = CommandStream(1024 * 1024);	// 1 Mb
		}

		m_FrameResourcesIndex = 0;

		m_Allocator.Init();
		m_CommandListManager.Init();
	}

	void D3D12RHI::Shutdown()
	{
		for (auto& queue : m_ResourceFreeQueues)
		{
			queue.Flush();
		}

		m_CommandListManager.Destroy();
		m_Allocator.Destroy();
	}

	void D3D12RHI::BeginFrame(const Ref<Window>& window)
	{
		TRACY_PROFILE_FUNC();

		static uint32 maxFramesInFlight = ConsoleManager::Get().FindCVar("r.MaxFramesInFlight")->GetInt();
		m_FrameResourcesIndex = (m_FrameResourcesIndex + 1) % maxFramesInFlight;

		if (window->GetSwapChain())
			window->GetSwapChain().As<D3D12SwapChain>()->NewFrame();

		{
			TRACY_PROFILE_SCOPE("ResourceFreeQueue::Flush");
			m_ResourceFreeQueues[m_FrameResourcesIndex].Flush();
		}

		m_CommandListManager.NewFrame();
	}

	void D3D12RHI::EndFrame(const Ref<Window>& window)
	{
		TRACY_PROFILE_FUNC();

		if (window->GetSwapChain())
			window->GetSwapChain().As<D3D12SwapChain>()->Present();
	}

	Ref<Texture> D3D12RHI::CreateTexture(const TextureCreateInfo& info)
	{
		return Ref<D3D12Texture>::Create(info);
	}

	Ref<TextureView> D3D12RHI::CreateTextureView(const TextureViewCreateInfo& info, Texture* texture)
	{
		return nullptr;
	}

	Ref<VertexBuffer> D3D12RHI::CreateVertexBuffer(const String& name, uint64 size)
	{
		return Ref<D3D12VertexBuffer>::Create(name, size);
	}

	Ref<IndexBuffer> D3D12RHI::CreateIndexBuffer(const String& name, uint64 size, IndexSize indexSize)
	{
		return Ref<D3D12IndexBuffer>::Create(name, size, indexSize);
	}

	Ref<CommandList> D3D12RHI::CreateCommandList()
	{
		return nullptr;
	}

	Ref<TracyGPUZone> D3D12RHI::CreateTracyZone(const Ref<CommandList>& cmdList, uint32 line, std::string_view fileName, std::string_view functionName, const String& name)
	{
		return nullptr;
	}

	Ref<SwapChain> D3D12RHI::CreateSwapChain(void* windowHandle, uint32 width, uint32 height, bool vsync)
	{
		return Ref<D3D12SwapChain>::Create(windowHandle, width, height, vsync);
	}

	Ref<ImGuiBackend> D3D12RHI::CreateImGuiBackend()
	{
		return Ref<D3D12ImGuiBackend>::Create();
	}

	uint64 D3D12RHI::GetMemoryUsage()
	{
		DXGI_QUERY_VIDEO_MEMORY_INFO memoryInfo = {};
		D3D12_CHECK(m_Adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &memoryInfo));

		return memoryInfo.CurrentUsage / 1024;
	}

	const GPUCapabilities& D3D12RHI::GetGPUCapabilities()
	{
		return m_GPUCapabilities;
	}

	const FormatSupportProperties& D3D12RHI::D3D12RHI::GetFormatSupportProperties(Format format)
	{
		return m_GPUCapabilities.Formats[format];
	}

	void D3D12RHI::QueryDeviceFeatures()
	{
		D3D_FEATURE_LEVEL featureLevels[] = {
			D3D_FEATURE_LEVEL_1_0_GENERIC,
			D3D_FEATURE_LEVEL_1_0_CORE,
			D3D_FEATURE_LEVEL_9_1,
			D3D_FEATURE_LEVEL_9_2,
			D3D_FEATURE_LEVEL_9_3,
			D3D_FEATURE_LEVEL_10_0,
			D3D_FEATURE_LEVEL_10_1,
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_12_0,
			D3D_FEATURE_LEVEL_12_1,
			D3D_FEATURE_LEVEL_12_2
		};

		D3D12_FEATURE_DATA_FEATURE_LEVELS featureLevelInfo = {};
		featureLevelInfo.NumFeatureLevels = ATN_ARRAY_SIZE(featureLevels);
		featureLevelInfo.pFeatureLevelsRequested = featureLevels;

		D3D12_CHECK(m_Device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featureLevelInfo, sizeof(featureLevelInfo)));

		m_FeaturesInfo.FeatureLevel = featureLevelInfo.MaxSupportedFeatureLevel;

		D3D_SHADER_MODEL shaderModels[] = {
			D3D_SHADER_MODEL_6_9,
			D3D_SHADER_MODEL_6_8,
			D3D_SHADER_MODEL_6_7,
			D3D_SHADER_MODEL_6_6,
			D3D_SHADER_MODEL_6_5,
			D3D_SHADER_MODEL_6_4,
			D3D_SHADER_MODEL_6_3,
			D3D_SHADER_MODEL_6_2,
			D3D_SHADER_MODEL_6_1,
			D3D_SHADER_MODEL_6_0,
			D3D_SHADER_MODEL_5_1
		};

		for (D3D_SHADER_MODEL shaderModel : shaderModels)
		{
			D3D12_FEATURE_DATA_SHADER_MODEL shaderModelInfo = { shaderModel };

			if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModelInfo, sizeof(shaderModelInfo))))
			{
				m_FeaturesInfo.ShaderModel = shaderModelInfo.HighestShaderModel;
				break;
			}
		}

		D3D_ROOT_SIGNATURE_VERSION rootSignatureVersions[] = {
			D3D_ROOT_SIGNATURE_VERSION_1_2,
			D3D_ROOT_SIGNATURE_VERSION_1_1,
			D3D_ROOT_SIGNATURE_VERSION_1_0,
			D3D_ROOT_SIGNATURE_VERSION_1,
		};

		for (D3D_ROOT_SIGNATURE_VERSION rootSignature : rootSignatureVersions)
		{
			D3D12_FEATURE_DATA_ROOT_SIGNATURE rootSignatureInfo = { rootSignature };

			if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &rootSignatureInfo, sizeof(rootSignatureInfo))))
			{
				m_FeaturesInfo.RootSignatureVersion = rootSignatureInfo.HighestVersion;
				break;
			}
		}

		D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
		if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5))))
		{
			m_FeaturesInfo.RayTracingTier = options5.RaytracingTier;
			m_FeaturesInfo.RenderPassesTier = options5.RenderPassesTier;
		}

		D3D12_FEATURE_DATA_D3D12_OPTIONS18 options18 = {};
		if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS18, &options18, sizeof(options18))))
		{
			m_FeaturesInfo.RenderPassesValid = options18.RenderPassesValid;
		}

		if (!m_FeaturesInfo.RenderPassesValid)
		{
			ATN_LOG_FATAL(D3D12, "DirectX 12 render passes are not supported on current system (try to update drivers or use Vulkan instead)!");
		}


		for (uint32 i = 0; i < ATN_ARRAY_SIZE(m_GPUCapabilities.Formats); ++i)
		{
			Format format = Format(i);
			FormatSupportProperties& properties = m_GPUCapabilities.Formats[format];

			if (format == Format::Unknown || format == Format::FormatNum)
			{
				properties.IsTextureSupported = false;
				properties.IsRenderTargetSupported = false;
				properties.IsVertexBufferSupported = false;
			}
			else
			{
				D3D12_FEATURE_DATA_FORMAT_SUPPORT formatSupport = {};
				formatSupport.Format = D3D12::TranslateFormat(format);

				D3D12_CHECK(m_Device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &formatSupport, sizeof(formatSupport)));

				properties.IsTextureSupported = (formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_TEXTURE2D) &&
					(formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_TEXTURECUBE) &&
					(formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE);

				properties.IsRenderTargetSupported = (formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_RENDER_TARGET) ||
					(formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL);

				properties.IsVertexBufferSupported = (formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_IA_VERTEX_BUFFER) != 0;
			}
		}

		DXGI_ADAPTER_DESC1 desc;
		m_Adapter->GetDesc1(&desc);

		DXGI_QUERY_VIDEO_MEMORY_INFO memoryInfo = {};
		D3D12_CHECK(m_Adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &memoryInfo));

		m_GPUCapabilities.Name = ToString(desc.Description);	
		m_GPUCapabilities.VRAM = memoryInfo.Budget;

		m_GPUCapabilities.MaxTextureDimension2D = 16384;
		m_GPUCapabilities.MaxTextureDimensionCube = 16384;
		m_GPUCapabilities.MaxFramebufferWidth = 16384;
		m_GPUCapabilities.MaxFramebufferHeight = 16384;
		m_GPUCapabilities.MaxViewportWidth = 32768;
		m_GPUCapabilities.MaxViewportHeight = 32768;
		m_GPUCapabilities.MaxSamplerLodBias = 16.f;
		m_GPUCapabilities.MaxSamplerAnisotropy = 16.f;
	}
}
