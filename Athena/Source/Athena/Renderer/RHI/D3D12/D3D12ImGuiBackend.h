#pragma once

#include "Athena/Core/Core.h"
#include "Athena/UI/ImGuiBackend.h"
#include "Athena/Renderer/Texture.h"

#include "imgui_threaded_rendering.h"


namespace Athena
{
	class ATHENA_API D3D12ImGuiBackend : public ImGuiBackend
	{
	public:
		D3D12ImGuiBackend();
		virtual ~D3D12ImGuiBackend();

		virtual void Init_RT() override;
		virtual void Init_GT(void* windowHandle) override;

		virtual void NewFrame_RT() override;
		virtual void NewFrame_GT() override;
		virtual void SwapDrawData() override;
		virtual void RenderDrawData() override;

		virtual void* GetTextureID(const Ref<TextureView>& texture) override;

		virtual void OnSwapChainRecreate() override;
	private:

	};
}
