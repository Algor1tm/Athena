#include "Material.h"


namespace Athena
{
	Material::Material()
	{

	}

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

		if (m_ShaderData.UseAlbedoTexture = m_Description.UseAlbedoTexture && m_Description.AlbedoTexture)
			m_Description.AlbedoTexture->Bind();

		if (m_ShaderData.UseNormalMap = m_Description.UseNormalMap && m_Description.NormalMap)
			m_Description.NormalMap->Bind();

		if (m_ShaderData.UseRoughnessMap = m_Description.UseRoughnessMap && m_Description.Roughness)
			m_Description.RoughnessMap->Bind();

		if (m_ShaderData.UseMetalnessMap = m_Description.UseMetalnessMap && m_Description.MetalnessMap)
			m_Description.MetalnessMap->Bind();

		return m_ShaderData;
	}
}
