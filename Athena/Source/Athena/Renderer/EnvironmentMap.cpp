#include "EnvironmentMap.h"

#include "Athena/Renderer/Texture.h"
#include "Athena/Renderer/SceneRenderer.h"


namespace Athena
{
	Ref<EnvironmentMap> EnvironmentMap::Create(const FilePath& hdrMap)
	{
		Ref<EnvironmentMap> result = CreateRef<EnvironmentMap>();
		result->m_FilePath = hdrMap;

		result->Load();

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

	void EnvironmentMap::SetResolution(uint32 resolution)
	{
		m_Resolution = resolution;
		Load();
	}

	void EnvironmentMap::Load()
	{
		Ref<Texture2D> equirectangularMap = Texture2D::Create(m_FilePath);

		if (equirectangularMap->IsLoaded())
		{
			SceneRenderer::PreProcessEnvironmentMap(equirectangularMap, this);
		}
	}
}
