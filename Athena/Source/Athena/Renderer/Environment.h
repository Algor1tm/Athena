#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Renderer/EnvironmentMap.h"
#include "Athena/Renderer/Light.h"


namespace Athena
{
	struct Environment
	{
		Ref<EnvironmentMap> EnvironmentMap;
		float AmbientLightIntensity = 1.f;
		float EnvironmentMapLOD = 0;

		float Exposure = 1;
		float Gamma = 2.2f;
	};
}
