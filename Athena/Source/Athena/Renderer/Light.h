#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Math/Vector.h"

#include "Athena/Renderer/Color.h"
#include "Athena/Renderer/EnvironmentMap.h"


namespace Athena
{
	struct DirectionalLight
	{
		Vector4 Color;
		Vector3 Direction;
		float Intensity;
		float LightSize;
		int CastShadows;
	};

	struct PointLight
	{
		Vector4 Color;
		Vector3 Position;
		float Intensity;
		float Radius;
		float FallOff;
	};

	struct LightEnvironment
	{
		std::vector<DirectionalLight> DirectionalLights;
		std::vector<PointLight> PointLights;

		Ref<EnvironmentMap> EnvironmentMap;
		float EnvironmentMapIntensity = 1.f;
		float EnvironmentMapLOD = 0;
	};
}
