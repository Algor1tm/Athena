#include "EnvironmentMap.h"

#include "Athena/Renderer/Texture.h"
#include "Athena/Renderer/Renderer.h"


namespace Athena
{
	Ref<EnvironmentMap> EnvironmentMap::Create(const FilePath& hdrMap)
	{
		Ref<EnvironmentMap> result = CreateRef<EnvironmentMap>();
		result->m_FilePath = hdrMap;

		Texture2DDescription desc;
		desc.TexturePath = hdrMap;
		Ref<Texture2D> equirectangularMap = Texture2D::Create(desc);

		if (equirectangularMap->IsLoaded())
		{
			Renderer::PreProcessEnvironmentMap(equirectangularMap, result->m_PrefilteredMap, result->m_IrradianceMap);
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
