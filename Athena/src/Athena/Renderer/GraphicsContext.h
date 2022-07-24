#pragma once


namespace Athena
{
	class ATHENA_API GraphicsContext
	{
	public:
		virtual void Init() = 0;
		virtual void SwapBuffers() = 0;
	};

}
