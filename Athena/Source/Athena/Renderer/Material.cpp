#include "Material.h"


namespace Athena
{
	Material::Material()
	{
		m_ConstantBuffer = ConstantBuffer::Create(sizeof(ShaderData), 2);
	}

	Ref<Material> Material::Create(const MaterialDescription& desc)
	{
		Ref<Material> result = CreateRef<Material>();
		result->m_Description = desc;
		return result;
	}

	void Material::Bind() 
	{
		m_ShaderData.Albedo = m_Description.Albedo;
		m_ShaderData.Roughness = m_Description.Roughness;
		m_ShaderData.Metalness = m_Description.Metalness;

		if (m_ShaderData.UseAlbedoTexture = m_Description.UseAlbedoTexture)
			m_Description.AlbedoTexture->Bind();

		if (m_ShaderData.UseNormalMap = m_Description.UseNormalMap)
			m_Description.NormalMap->Bind();

		if (m_ShaderData.UseRoughnessMap = m_Description.UseRoughnessMap)
			m_Description.RoughnessMap->Bind();

		if (m_ShaderData.UseMetalnessMap = m_Description.UseMetalnessMap)
			m_Description.MetalnessMap->Bind();

		m_ConstantBuffer->SetData(&m_ShaderData, sizeof(ShaderData));
	}
}
