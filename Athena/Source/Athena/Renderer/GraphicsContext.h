#pragma once

#include "Athena/Core/Core.h"


namespace Athena
{
	class ATHENA_API GraphicsContext
	{
	public:
		virtual ~GraphicsContext() = default;

		virtual void SwapBuffers() = 0;
		virtual void SetVSync(bool enabled) = 0;
		virtual void SetFullscreen(bool enabled) = 0;

		static Ref<GraphicsContext> Create(void* windowHandle);
	};
}
