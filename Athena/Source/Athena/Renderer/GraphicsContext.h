#pragma once

#include "Athena/Core/Core.h"


namespace Athena
{
	struct GPUInfo
	{
		String APIVersion;
		String GPUBrandString;
		String Vendor;

		uint64 TotalPhysicalMemoryKB = 0;
	};

	class ATHENA_API GraphicsContext
	{
	public:
		static Ref<GraphicsContext> Create(void* windowHandle);
		virtual ~GraphicsContext() = default;

		virtual void SwapBuffers() = 0;
		virtual void SetVSync(bool enabled) = 0;
		virtual void SetFullscreen(bool enabled) = 0;

		virtual void GetGPUInfo(GPUInfo* info) const = 0;
	};
}
