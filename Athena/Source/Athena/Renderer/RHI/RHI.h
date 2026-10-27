#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Window.h"
#include "Athena/Renderer/RenderThread.h"
#include "Athena/Renderer/CommandList.h"
#include "Athena/Renderer/Format.h"
#include "Athena/Renderer/GPUBuffers.h"
#include "Athena/Renderer/Texture.h"
#include "Athena/Renderer/SwapChain.h"
#include "Athena/UI/ImGuiBackend.h"


namespace Athena
{
	struct GPUCapabilities
	{
		String Name;
		uint64 VRAM;

		FormatSupportProperties Formats[Format::FormatNum - 1];

		uint32 MaxTextureDimension2D;
		uint32 MaxTextureDimensionCube;
		uint32 MaxFramebufferWidth;
		uint32 MaxFramebufferHeight;
		uint32 MaxViewportWidth;
		uint32 MaxViewportHeight;

		float  MaxSamplerLodBias;
		float  MaxSamplerAnisotropy;
	};

	class ATHENA_API RHI
	{
	public:
		static RHI* Get() { return s_RHI; }

		virtual ~RHI() = default;

		virtual void Init(const RendererConfig& config) = 0;

		virtual void Shutdown() = 0;

		virtual void BeginFrame(const Ref<Window>& window) = 0;

		virtual void EndFrame(const Ref<Window>& window) = 0;

		virtual Ref<Texture> CreateTexture(const TextureCreateInfo& info) = 0;

		virtual Ref<TextureView> CreateTextureView(const TextureViewCreateInfo& info, Texture* texture) = 0;

		virtual Ref<VertexBuffer> CreateVertexBuffer(const String& name, uint64 size) = 0;

		virtual Ref<IndexBuffer> CreateIndexBuffer(const String& name, uint64 size, IndexSize indexSize) = 0;

		virtual Ref<CommandList> CreateCommandList() = 0;

		virtual Ref<TracyGPUZone> CreateTracyZone(const Ref<CommandList>& cmdList, uint32 line, std::string_view fileName, std::string_view functionName, const String& name) = 0;

		virtual Ref<SwapChain> CreateSwapChain(void* windowHandle, uint32 width, uint32 height, bool vsync) = 0;

		virtual Ref<ImGuiBackend> CreateImGuiBackend() = 0;

		virtual uint64 GetMemoryUsage() = 0; // Kbs

		virtual const GPUCapabilities& GetGPUCapabilities() = 0;

		virtual const FormatSupportProperties& GetFormatSupportProperties(Format format) = 0;

		friend class RenderThread;

	private:
		// Initialized and destroyed in RenderThread class
		static inline RHI* s_RHI = nullptr;
	};
}
