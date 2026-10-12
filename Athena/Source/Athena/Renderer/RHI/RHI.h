#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/RenderThread.h"
#include "Athena/Renderer/Format.h"

namespace Athena
{
	struct GPUCapabilities
	{
		String Name;
		uint64 VRAM;

		FormatProperties Formats[Format::FormatNum - 1];

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

		virtual const GPUCapabilities& GetGPUCapabilities() = 0;
		virtual const FormatProperties& GetFormatProperties(Format format) = 0;

		friend class RenderThread;

	private:
		// Initialized and destroyed in RenderThread class
		static inline RHI* s_RHI = nullptr;
	};
}
