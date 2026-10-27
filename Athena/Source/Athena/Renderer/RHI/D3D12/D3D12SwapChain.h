#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/SwapChain.h"
#include "Athena/Renderer/RHI/D3D12/D3D12RHI.h"


namespace Athena
{
	class D3D12SwapChain : public SwapChain
	{
	public:
		D3D12SwapChain(void* windowHandle, uint32 width, uint32 height, bool vsync = false);
		virtual ~D3D12SwapChain();

		virtual void Init() override;

		virtual void OnWindowResized(uint32 width, uint32 height) override;
		virtual void SetVSync(bool enabled) override;

		void NewFrame();
		void Present();

	private:
		ComPtr<IDXGISwapChain4> m_DXGISwapChain;
		uint32 m_Width;
		uint32 m_Height;
		DXGI_FORMAT m_Format;
		
		uint32 m_BackBufferIndex;
		bool m_VSync;
		void* m_WindowHandle;
		bool m_TearingSupported;
	};
}
