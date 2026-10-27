#include "D3D12SwapChain.h"

#include "Athena/Core/Stats.h"
#include "Athena/Core/Application.h"

#include "Athena/Renderer/RHI/D3D12/D3D12Utils.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>


namespace Athena
{
	EXTERN_CYCLE_STAT(STAT_WaitForGPU);

	D3D12SwapChain::D3D12SwapChain(void* windowHandle, uint32 width, uint32 height, bool vsync)
		: m_WindowHandle(windowHandle),
		m_VSync(vsync),
		m_Width(width),
		m_Height(height),
		m_BackBufferIndex(0),
		m_TearingSupported(false)
	{

	}

	D3D12SwapChain::~D3D12SwapChain()
	{

	}

	void D3D12SwapChain::Init()
	{
		BOOL allowTearing = FALSE;
		if (SUCCEEDED(D3D12RHI::Get()->GetDXGIFactory()->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing))))
		{
			m_TearingSupported = allowTearing;
		}


		// Hardcode to this format for now
		m_Format = DXGI_FORMAT_R8G8B8A8_UNORM;

		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.Width = m_Width;
		swapChainDesc.Height = m_Height;
		swapChainDesc.Format = m_Format;
		swapChainDesc.Stereo = false;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = D3D12::GetMaxFramesInFlight();
		swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
		swapChainDesc.Flags = m_TearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

		HWND hwnd = glfwGetWin32Window((GLFWwindow*)m_WindowHandle);

		ComPtr<IDXGISwapChain1> swapChain1;
		D3D12_CHECK(D3D12RHI::Get()->GetDXGIFactory()->CreateSwapChainForHwnd(
			D3D12RHI::Get()->GetQueue().Get(),
			hwnd,
			&swapChainDesc,
			nullptr,
			nullptr,
			swapChain1.GetAddressOf()
		));

		D3D12_CHECK(swapChain1.As(&m_DXGISwapChain));
	}

	void D3D12SwapChain::OnWindowResized(uint32 width, uint32 height)
	{
		D3D12_CHECK(m_DXGISwapChain->ResizeBuffers(
			D3D12::GetMaxFramesInFlight(), 
			width,
			height,
			m_Format,
			m_TearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0
		));
	}

	void D3D12SwapChain::SetVSync(bool enabled)
	{
		m_VSync = enabled;
	}

	void D3D12SwapChain::NewFrame()
	{
		{
			TRACY_PROFILE_SCOPE("WaitForGPU");
			SCOPE_CYCLE_STAT(STAT_WaitForGPU);

			D3D12CommandList* presentCommandList = D3D12RHI::Get()->GetCmdListManager()->GetPresentCommandList();
			D3D12RHI::Get()->GetCmdListManager()->WaitForCmdList(presentCommandList);
		}

		m_BackBufferIndex = m_DXGISwapChain->GetCurrentBackBufferIndex();
	}

	void D3D12SwapChain::Present()
	{
		UINT syncInterval = m_VSync ? 1 : 0;
		UINT presentFlags = (!m_VSync && m_TearingSupported) ? DXGI_PRESENT_ALLOW_TEARING : 0;

		D3D12_CHECK(m_DXGISwapChain->Present(syncInterval, presentFlags));
	}
}
