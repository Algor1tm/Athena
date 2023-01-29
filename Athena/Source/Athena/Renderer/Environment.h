#pragma once

#include "Athena/Core/Core.h"

#include "Skybox.h"
#include "Texture.h"
#include "Light.h"


namespace Athena
{
	struct Environment
	{
		DirectionalLight DirLight;

		Ref<Skybox> Skybox;

		float SkyboxLOD = 0;
		float Exposure = 1;
	};
}
