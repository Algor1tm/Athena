#pragma once

#include "Athena/Core/Core.h"

#include "Texture.h"
#include "Light.h"


namespace Athena
{
	struct Environment
	{
		DirectionalLight DirLight;

		Ref<Cubemap> Skybox;
		Ref<Cubemap> IrradianceMap;
		Ref<Cubemap> PrefilterMap;

		float SkyboxLOD = 0;
		float Exposure = 1;
	};
}
