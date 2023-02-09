#include "Skybox.h"

#include "Renderer.h"


namespace Athena
{
	Ref<Skybox> Skybox::Create(const FilePath& hdrMap)
	{
		Ref<Skybox> result = CreateRef<Skybox>();
		result->m_FilePath = hdrMap;

		Texture2DDescription desc;
		desc.TexturePath = hdrMap;
		Ref<Texture2D> equirectangularMap = Texture2D::Create(desc);

		if (equirectangularMap->IsLoaded())
		{
			Renderer::PreProcessEnvironmentMap(equirectangularMap, result->m_SkyboxMap, result->m_IrradianceMap);
		}

		return result;
	}

	void Skybox::Bind()
	{
		if (m_SkyboxMap)
		{
			m_SkyboxMap->Bind(TextureBinder::SKY_BOX);
			m_IrradianceMap->Bind(TextureBinder::IRRADIANCE_MAP);
		}
	}
}
