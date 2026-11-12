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
		D3D12RHI::Get()->DeleteFrameResourcesDeferred([buffers = m_BackBuffers, swapChain = m_DXGISwapChain, fence = m_Fence]() mutable
			{
				for (UINT i = 0; i < buffers.size(); i++)
				{
					buffers[i]->Release();
				}

				swapChain->Release();
				fence.Destroy();
			});
	}

	void D3D12SwapChain::Init()
	{
		BOOL allowTearing = FALSE;
		if (SUCCEEDED(D3D12RHI::Get()->GetDXGIFactory()->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing))))
		{
			m_TearingSupported = allowTearing;
		}

		m_GraphicsContext = RHI::Get()->GetContext(CommandContextType::Graphics).As<D3D12CommandContext>();

		m_Fence.Init();
		m_FenceFrameValues.resize(D3D12::GetMaxFramesInFlight(), m_Fence.GetCompletedValue());
		D3D12_SET_DEBUG_NAME(m_Fence.Fence, "SwapChainFence");

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

		IDXGISwapChain1* swapChain1;
		D3D12_CHECK(D3D12RHI::Get()->GetDXGIFactory()->CreateSwapChainForHwnd(
			m_GraphicsContext->GetQueue(),
			hwnd,
			&swapChainDesc,
			nullptr,
			nullptr,
			&swapChain1
		));

		D3D12_CHECK(swapChain1->QueryInterface(&m_DXGISwapChain));
		swapChain1->Release();

		m_BackBuffers.resize(swapChainDesc.BufferCount);
		for (UINT i = 0; i < m_BackBuffers.size(); i++)
		{
			m_DXGISwapChain->GetBuffer(i, IID_PPV_ARGS(&m_BackBuffers[i]));
		}

		auto uiLayer = Application::Get().GetImGuiLayer();
		if (uiLayer)
			uiLayer->OnSwapChainRecreate();
	}	

	void D3D12SwapChain::OnWindowResized(uint32 width, uint32 height)
	{
		TRACY_PROFILE_FUNC();

		D3D12RHI::Get()->WaitDeviceIdle();

		for (UINT i = 0; i < m_BackBuffers.size(); i++)
		{
			m_BackBuffers[i]->Release();
		}

		D3D12_CHECK(m_DXGISwapChain->ResizeBuffers(
			m_BackBuffers.size(),
			width,
			height,
			m_Format,
			m_TearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0
		));

		for (UINT i = 0; i < m_BackBuffers.size(); i++)
		{
			m_DXGISwapChain->GetBuffer(i, IID_PPV_ARGS(&m_BackBuffers[i]));
			D3D12_SET_DEBUG_NAME(m_BackBuffers[i], fmt::format("BackBuffer_{}", i));
		}

		auto uiLayer = Application::Get().GetImGuiLayer();
		if (uiLayer)
			uiLayer->OnSwapChainRecreate();
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

			uint64 fenceValue = m_FenceFrameValues[D3D12RHI::Get()->GetFrameResourcesIndex()];
			if(fenceValue != 0)
				m_Fence.WaitValue(fenceValue);
		}

		m_BackBufferIndex = m_DXGISwapChain->GetCurrentBackBufferIndex();
	}

	void D3D12SwapChain::Present()
	{
		TRACY_PROFILE_FUNC();

		m_Fence.Signal(m_GraphicsContext->GetQueue());
		m_FenceFrameValues[D3D12RHI::Get()->GetFrameResourcesIndex()] = m_Fence.GetSignaledValue();

		UINT syncInterval = m_VSync ? 1 : 0;
		UINT presentFlags = (!m_VSync && m_TearingSupported) ? DXGI_PRESENT_ALLOW_TEARING : 0;

		D3D12_CHECK(m_DXGISwapChain->Present(syncInterval, presentFlags));

#if ATN_ENABLE_TRACY_GPU
		m_GraphicsContext->TracyCollect();
#endif
	}
}
