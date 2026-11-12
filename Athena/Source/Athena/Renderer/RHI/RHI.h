#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Window.h"
#include "Athena/Renderer/RenderThread.h"
#include "Athena/Renderer/CommandContext.h"
#include "Athena/Renderer/Format.h"
#include "Athena/Renderer/GPUBuffers.h"
#include "Athena/Renderer/Texture.h"
#include "Athena/Renderer/Shader.h"
#include "Athena/Renderer/ShaderCompiler.h"
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

		virtual Ref<CommandContext> GetContext(CommandContextType type) = 0;

		virtual void ForEachUniqueContext(const std::function<void(const Ref<CommandContext>&)>&) = 0;

		virtual Ref<Texture> CreateTexture(const TextureCreateInfo& info) = 0;

		virtual Ref<TextureView> CreateTextureView(const TextureViewCreateInfo& info, Texture* texture) = 0;

		virtual Ref<GPUBuffer> CreateGPUBuffer(const GPUBufferCreateInfo& info) = 0;

		virtual Ref<ShaderCompiler> CreateShaderCompiler() = 0;

		virtual Ref<ShaderInstance> CreateShaderInstance(const Buffer& bytecode, const ShaderReflectionData& reflectionData) = 0;

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
