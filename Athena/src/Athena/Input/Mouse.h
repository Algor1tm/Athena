#pragma once

#include "Athena/Core/Core.h"


namespace Athena
{
	namespace Mouse
	{
		enum Button: uint16
		{
			// From glfw3.h
			XButton0 = 0,
			XButton1 = 1,
			XButton2 = 2,
			XButton3 = 3,
			XButton4 = 4,
			XButton5 = 5,
			XButton6 = 6,
			XButton7 = 7,

			LastButton = XButton7,
			Left = XButton0,
			Right = XButton1,
			Wheel = XButton2
		};
	}
}
