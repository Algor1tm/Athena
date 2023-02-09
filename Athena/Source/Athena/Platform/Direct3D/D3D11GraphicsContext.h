#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Renderer/GraphicsContext.h"

#include <d3d11.h>
#include <wrl.h>


namespace Athena
{
	class ATHENA_API D3D11GraphicsContext: public GraphicsContext
	{
	public:
		friend class D3D11CurrentContext;

	public:
		D3D11GraphicsContext(HWND hWnd);
		virtual ~D3D11GraphicsContext();

		virtual void SwapBuffers() override;
		virtual void SetVSync(bool enabled) override;
		virtual void SetFullscreen(bool enabled) override;

	private:
		Microsoft::WRL::ComPtr<ID3D11Device> m_Device;
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_DeviceContext;
		Microsoft::WRL::ComPtr<IDXGISwapChain> m_SwapChain;

		uint32 m_Interval = 1;
	};

	class ATHENA_API D3D11CurrentContext
	{
	public:
		static void SetCurrentContext(const D3D11GraphicsContext& context);

		static Microsoft::WRL::ComPtr<ID3D11Device> Device;
		static Microsoft::WRL::ComPtr<ID3D11DeviceContext> DeviceContext;
		static Microsoft::WRL::ComPtr<IDXGISwapChain> SwapChain;
	};
}

