#include "atnpch.h"
#include "D3D11ImGuiLayerImpl.h"

#include "D3D11GraphicsContext.h"

#ifdef _MSC_VER
#pragma warning(push, 0)
#endif

#define IMGUI_API_IMPL
#include <ImGui/backends/imgui_impl_win32.h>
#include <ImGui/backends/imgui_impl_dx11.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif


namespace Athena
{
	void D3D11ImGuiLayerImpl::Init(void* windowHandle)
	{
		ImGui_ImplWin32_Init(reinterpret_cast<HWND>(windowHandle));
		ImGui_ImplDX11_Init(D3D11CurrentContext::Device.Get(), D3D11CurrentContext::DeviceContext.Get());
	}

	void D3D11ImGuiLayerImpl::Shutdown()
	{
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
	}

	void D3D11ImGuiLayerImpl::NewFrame()
	{
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
	}

	void D3D11ImGuiLayerImpl::RenderDrawData()
	{
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	}

	void D3D11ImGuiLayerImpl::UpdateViewports()
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}
}
