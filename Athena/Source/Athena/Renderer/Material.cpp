#include "Material.h"

#include "Renderer.h"


namespace Athena
{
	Ref<Material> Material::Create(const MaterialDescription& desc)
	{
		Ref<Material> result = CreateRef<Material>();
		result->m_Description = desc;
		return result;
	}

	const Material::ShaderData& Material::Bind()
	{
		m_ShaderData.Albedo = m_Description.Albedo;
		m_ShaderData.Roughness = m_Description.Roughness;
		m_ShaderData.Metalness = m_Description.Metalness;
		m_ShaderData.AmbientOcclusion = m_Description.AmbientOcclusion;

		if (m_ShaderData.UseAlbedoTexture = m_Description.UseAlbedoTexture && m_Description.AlbedoTexture)
			m_Description.AlbedoTexture->Bind(TextureBinder::ALBEDO_TEXTURE);

		if (m_ShaderData.UseNormalMap = m_Description.UseNormalMap && m_Description.NormalMap)
			m_Description.NormalMap->Bind(TextureBinder::NORMAL_MAP);

		if (m_ShaderData.UseRoughnessMap = m_Description.UseRoughnessMap && m_Description.RoughnessMap)
			m_Description.RoughnessMap->Bind(TextureBinder::ROUGHNESS_MAP);

		if (m_ShaderData.UseMetalnessMap = m_Description.UseMetalnessMap && m_Description.MetalnessMap)
			m_Description.MetalnessMap->Bind(TextureBinder::METALNESS_MAP);

		if (m_ShaderData.UseAmbientOcclusionMap = m_Description.UseAmbientOcclusionMap && m_Description.AmbientOcclusionMap)
			m_Description.AmbientOcclusionMap->Bind(TextureBinder::AMBIENT_OCCLUSION_MAP);

		return m_ShaderData;
	}
}
