#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Texture.h"

namespace Athena
{
	class ATHENA_API ImGuiBackend: public RefCounted
	{
	public:
		static Ref<ImGuiBackend> Create(void* windowHandle);
		virtual ~ImGuiBackend() = default;

		virtual void Init(void* windowHandle) = 0;

		virtual void NewFrame() = 0;
		virtual void RenderDrawData(uint32 width, uint32 height) = 0;
		virtual void RenderViewports() = 0;

		virtual void* GetTextureID(const Ref<TextureView>& texture) = 0;

		virtual void OnSwapChainRecreate() = 0;
	};
}