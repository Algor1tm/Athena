#pragma once

#include "Athena/Core/Core.h"


namespace Athena
{
	class ATHENA_API GraphicsContext
	{
	public:
		virtual ~GraphicsContext() = default;

		virtual void Init() = 0;
		virtual void SwapBuffers() = 0;

		static Ref<GraphicsContext> Create(void* windowHandle);
	};
}
