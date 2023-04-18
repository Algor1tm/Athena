#include "EnvironmentMap.h"

#include "Athena/Renderer/Texture.h"
#include "Athena/Renderer/SceneRenderer.h"


namespace Athena
{
	Ref<EnvironmentMap> EnvironmentMap::Create(const FilePath& hdrMap)
	{
		Ref<EnvironmentMap> result = CreateRef<EnvironmentMap>();
		result->m_FilePath = hdrMap;

		Ref<Texture2D> equirectangularMap = Texture2D::Create(hdrMap);

		if (equirectangularMap->IsLoaded())
		{
			SceneRenderer::PreProcessEnvironmentMap(equirectangularMap, result->m_PrefilteredMap, result->m_IrradianceMap);
		}

		return result;
	}

	void EnvironmentMap::Bind()
	{
		if (m_PrefilteredMap && m_IrradianceMap)
		{
			m_PrefilteredMap->Bind(TextureBinder::ENVIRONMENT_MAP);
			m_IrradianceMap->Bind(TextureBinder::IRRADIANCE_MAP);
		}
	}
}
