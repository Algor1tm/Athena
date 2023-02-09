#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Renderer/Skybox.h"
#include "Athena/Renderer/Light.h"


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
