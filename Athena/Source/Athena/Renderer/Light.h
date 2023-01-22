#pragma once

#include "Athena/Math/Vector.h"
#include "Color.h"


namespace Athena
{
	struct DirectionalLight
	{
		LinearColor Color = LinearColor::White;
		Vector3 Direction = Vector3(-1, -1, -1);
		float Intensity = 1;
	};

	struct PointLight
	{
		LinearColor Color = LinearColor::White;
		Vector3 Direction = Vector3::Forward();
		Vector3 Position = Vector3(0.f);
	};

	struct SpotLight
	{
		LinearColor Color = LinearColor::White;
		Vector3 Direction = Vector3::Forward();
		Vector3 Position = Vector3(0.f);
		float InnerCutOff = Math::Cos(Math::Radians(12.5f));
		float OuterCutOff = Math::Cos(Math::Radians(15.f));
	};
}
