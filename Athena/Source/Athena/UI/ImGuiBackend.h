#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Texture.h"

namespace Athena
{
	class ATHENA_API ImGuiBackend: public RefCounted
	{
	public:
		static Ref<ImGuiBackend> Create();
		virtual ~ImGuiBackend() = default;

		virtual void Init_RT() = 0;
		virtual void Init_GT(void* windowHandle) = 0;

		virtual void NewFrame_RT() = 0;
		virtual void NewFrame_GT() = 0;
		virtual void SwapDrawData() = 0;
		virtual void RenderDrawData() = 0;

		virtual void* GetTextureID(const Ref<TextureView>& texture) = 0;

		virtual void OnSwapChainRecreate() = 0;
	};
}