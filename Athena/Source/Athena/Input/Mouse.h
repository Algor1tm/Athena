#pragma once

#include "Athena/Core/Core.h"


namespace Athena
{
	namespace Mouse
	{
		enum Button : uint16
		{
			XButton1 = 0x0020,
			XButton2 = 0x0040,

			Left = 0x0001,
			Right = 0x0002,
			Middle = 0x0010
		};
	}
}
