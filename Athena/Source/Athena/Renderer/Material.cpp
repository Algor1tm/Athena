#include "Material.h"

#include "Renderer.h"


namespace Athena
{
	std::unordered_map<String, Ref<Material>> MaterialManager::m_Materials;


	Ref<Material> MaterialManager::CreateMaterial(const MaterialDescription& desc, const String& name)
	{
		Ref<Material> material = CreateRef<Material>();
		material->m_Description = desc;

		if (m_Materials.find(name) != m_Materials.end())
		{
			uint32 postfix = 0;
			String newName;
			do
			{
				newName = name + std::to_string(postfix);
				postfix++;
			} while (m_Materials.find(name) == m_Materials.end());

			material->m_Name = newName;
		}
		else
		{
			material->m_Name = name;
		}

		m_Materials[material->m_Name] = material;

		return material;
	}

	Ref<Material> MaterialManager::GetMaterial(const String& name)
	{
		if (m_Materials.find(name) == m_Materials.end())
		{
			ATN_CORE_ERROR("MaterialManager::GetMaterial: invalid material name!");
			return nullptr;
		}
		else
		{
			return m_Materials.at(name);
		}
	}

	void MaterialManager::DeleteMaterial(const String& name)
	{
		if (m_Materials.find(name) == m_Materials.end())
		{
			ATN_CORE_ERROR("MaterialManager::DeleteMaterial: invalid material name!");
		}
		else
		{
			m_Materials.erase(name);
		}
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
