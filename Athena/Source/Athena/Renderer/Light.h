#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Math/Vector.h"

#include "Athena/Renderer/Color.h"


namespace Athena
{
	struct DirectionalLight
	{
		Vector4 Color;
		Vector3 Direction;
		float Intensity;
	};

	struct PointLight
	{
		Vector4 Color;
		Vector3 Position;
		float Intensity;
		float Radius;
		float FallOff;
	};
}
