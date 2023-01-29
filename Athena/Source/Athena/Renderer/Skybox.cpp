#include "Skybox.h"

#include "Renderer.h"


namespace Athena
{
	Ref<Skybox> Skybox::Create(const Filepath& hdrMap)
	{
		Ref<Skybox> result = CreateRef<Skybox>();
		result->m_Path = hdrMap;

		Texture2DDescription desc;
		desc.TexturePath = hdrMap;
		desc.HDR = true;
		Ref<Texture2D> equirectangularMap = Texture2D::Create(desc);

		if (equirectangularMap->IsLoaded())
		{
			Renderer::PreProcessEnvironmentMap(equirectangularMap, result->m_SkyboxCubemap, result->m_IrradianceMap, result->m_PrefilterMap);
		}

		return result;
	}

	void Skybox::Bind()
	{
		if (m_SkyboxCubemap)
		{
			m_SkyboxCubemap->Bind(TextureBinder::SKY_BOX);
			m_IrradianceMap->Bind(TextureBinder::IRRADIANCE_MAP);
			m_PrefilterMap->Bind(TextureBinder::PREFILTER_MAP);
		}
	}
}
