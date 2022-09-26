#include "atnpch.h"
#include "D3D11GraphicsContext.h"

#include <vector>
#include <algorithm>


namespace Athena
{
	Microsoft::WRL::ComPtr<ID3D11Device> D3D11CurrentContext::Device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> D3D11CurrentContext::DeviceContext;
	Microsoft::WRL::ComPtr<IDXGISwapChain> D3D11CurrentContext::SwapChain;

	struct AdapterData
	{
		Microsoft::WRL::ComPtr<IDXGIAdapter> Adapter;
		DXGI_ADAPTER_DESC Description;
	};

	static std::vector<AdapterData> GetAdapters()
	{
		Microsoft::WRL::ComPtr<IDXGIFactory> pFactory;
		HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), reinterpret_cast<void**>(pFactory.GetAddressOf()));
		ATN_CORE_ASSERT(SUCCEEDED(hr), "Failed to CreateDXGIFactory()!");

		std::vector<AdapterData> adapters;

		Microsoft::WRL::ComPtr<IDXGIAdapter> pAdapter;
		UINT index = 0;
		while (SUCCEEDED(pFactory->EnumAdapters(index, pAdapter.GetAddressOf())))
		{
			AdapterData aData;
			aData.Adapter = pAdapter;
			pAdapter->GetDesc(&aData.Description);

			adapters.push_back(aData);
			index += 1;
		}

		return adapters;
	}

	static std::string_view D3DFeatureLevelToText(D3D_FEATURE_LEVEL level)
	{
		std::string_view version;
		switch (level)
		{
		case D3D_FEATURE_LEVEL_1_0_CORE: version = "1.0 Core"; break;
		case D3D_FEATURE_LEVEL_9_1: version = "9.1"; break;
		case D3D_FEATURE_LEVEL_9_2: version = "9.2"; break;
		case D3D_FEATURE_LEVEL_9_3: version = "9.3"; break;
		case D3D_FEATURE_LEVEL_10_0: version = "10.0"; break;
		case D3D_FEATURE_LEVEL_10_1: version = "10.1"; break;
		case D3D_FEATURE_LEVEL_11_0: version = "11.0"; break;
		case D3D_FEATURE_LEVEL_11_1: version = "11.1"; break;
		case D3D_FEATURE_LEVEL_12_0: version = "12.0"; break;
		case D3D_FEATURE_LEVEL_12_1: version = "12.1"; break;
		default: version = "Unknown";
		};

		return version;
	}

	static std::string_view D3DVendorIDToName(UINT vendorID)
	{
		if (vendorID == 0x10DE)
			return "NVIDIA";
		if (vendorID == 0x1002 || vendorID == 0x1022)
			return "AMD";
		if (vendorID == 0x163C || vendorID == 0x8086 || vendorID == 0x8087)
			return "INTEL";

		return "Unknown";
	}


	void D3D11CurrentContext::SetCurrentContext(const D3D11GraphicsContext& context)
	{
		Device = context.m_Device;
		DeviceContext = context.m_DeviceContext;
		SwapChain = context.m_SwapChain;
	}

	D3D11GraphicsContext::D3D11GraphicsContext(HWND hWnd)
	{
		RECT rect;
		GetClientRect(hWnd, &rect);

		DXGI_SWAP_CHAIN_DESC swapChainDesc;
		ZeroMemory(&swapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));
		swapChainDesc.BufferDesc.Width = rect.right - rect.left;
		swapChainDesc.BufferDesc.Height = rect.bottom - rect.top;
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

		swapChainDesc.SampleDesc.Count = 1;	
		swapChainDesc.SampleDesc.Quality = 0;

		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = 1;
		swapChainDesc.OutputWindow = hWnd;
		swapChainDesc.Windowed = TRUE;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

		auto adapters = GetAdapters();
		ATN_CORE_ASSERT(adapters.size() > 0, "Could not find any adapters!");
		std::sort(adapters.begin(), adapters.end(), [](const auto& left, const auto& right)
			{ 
				return left.Description.DedicatedVideoMemory > right.Description.DedicatedVideoMemory;
			});
		AdapterData& selectedAdapter = adapters[0];

#ifdef ATN_DEBUG
		UINT createFlags = D3D11_CREATE_DEVICE_DEBUG;
#else
		UINT createFlags = 0;
#endif
		D3D_FEATURE_LEVEL featureLevelArray[] =
		{
			D3D_FEATURE_LEVEL_11_1,
		};

		D3D_FEATURE_LEVEL featureLevel;

		HRESULT hr = D3D11CreateDeviceAndSwapChain(
			selectedAdapter.Adapter.Get(),
			D3D_DRIVER_TYPE_UNKNOWN,
			nullptr,
			createFlags,
			featureLevelArray,
			ARRAYSIZE(featureLevelArray),
			D3D11_SDK_VERSION,
			&swapChainDesc,
			m_SwapChain.GetAddressOf(),
			m_Device.GetAddressOf(),
			&featureLevel,
			m_DeviceContext.GetAddressOf()
		);
		ATN_CORE_ASSERT(SUCCEEDED(hr), "Failed to CreateDeviceAndSwapChain()!");

		ATN_CORE_INFO("");
		ATN_CORE_INFO("Create Direct3D Graphics Context:");
		ATN_CORE_INFO("Direct3D Version: {0}", D3DFeatureLevelToText(featureLevel));
		ATN_CORE_INFO("Graphics Card: {0}", Filepath(selectedAdapter.Description.Description).string());
		ATN_CORE_INFO("Vendor: {0}", D3DVendorIDToName(selectedAdapter.Description.VendorId));
		ATN_CORE_INFO("");

		D3D11CurrentContext::SetCurrentContext(*this);
	}

	D3D11GraphicsContext::~D3D11GraphicsContext()
	{
		m_SwapChain->SetFullscreenState(FALSE, nullptr);
	}

	void D3D11GraphicsContext::SwapBuffers()
	{
		static UINT presentFlags = 0;

		if (m_SwapChain->Present(m_Interval, presentFlags) == DXGI_STATUS_OCCLUDED)		// Simulate VSync when window is minimized
		{
			presentFlags = DXGI_PRESENT_TEST;
			Sleep(20);
		}
		else 
		{
			presentFlags = 0;
		}
	}

	void D3D11GraphicsContext::SetVSync(bool enabled)
	{
		if (enabled)
			m_Interval = 1;
		else
			m_Interval = 0;
	}

	void D3D11GraphicsContext::SetFullscreen(bool enabled)
	{
		D3D11CurrentContext::SwapChain->SetFullscreenState(enabled, nullptr);
	}
}
