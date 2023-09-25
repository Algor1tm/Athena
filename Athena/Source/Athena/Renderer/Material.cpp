#include "Material.h"

#include "Athena/Renderer/Texture.h"
#include "Athena/Renderer/Renderer.h"


namespace Athena
{
	std::unordered_map<String, Ref<Material>> MaterialManager::m_Materials;


	Ref<Material> MaterialManager::CreateMaterial(const String& name)
	{
		Ref<Material> material = CreateRef<Material>();

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
		ATN_CORE_WARN_TAG("Material", "Create material '{}'", name);

		return material;
	}

	bool MaterialManager::Exists(const String& name)
	{
		return m_Materials.find(name) != m_Materials.end();
	}

	Ref<Material> MaterialManager::Get(const String& name)
	{
		if (m_Materials.find(name) == m_Materials.end())
		{
			return nullptr;
		}
		else
		{
			return m_Materials.at(name);
		}
	}

	void MaterialManager::Delete(const String& name)
	{
		if (m_Materials.find(name) == m_Materials.end())
		{
			ATN_CORE_ERROR_TAG("MaterialManager", "Failed to delete material, invalid material name '{}'!", name);
		}
		else
		{
			m_Materials.erase(name);
			ATN_CORE_WARN_TAG("Material", "Delete material '{}'", name);
		}
	}

	void Material::Set(MaterialTexture textureType, const Ref<Texture2D>& texture)
	{
		m_TextureMap[textureType] = { texture, true };
	}

	Ref<Texture2D> Material::Get(MaterialTexture textureType)
	{
		if (m_TextureMap.find(textureType) == m_TextureMap.end())
			return nullptr;

		return m_TextureMap.at(textureType).Texture;
	}

	bool Material::IsEnabled(MaterialTexture textureType) const
	{
		if (m_TextureMap.find(textureType) == m_TextureMap.end())
			return false;

		return m_TextureMap.at(textureType).IsEnabled;
	}

	void Material::Enable(MaterialTexture textureType, bool enable)
	{
		if (m_TextureMap.find(textureType) != m_TextureMap.end())
			m_TextureMap.at(textureType).IsEnabled = enable;
	}

	const Material::ShaderData& Material::Bind()
	{
		BindTexture(MaterialTexture::ALBEDO_MAP, TextureBinder::ALBEDO_MAP, &m_ShaderData.EnableAlbedoMap);
		BindTexture(MaterialTexture::NORMAL_MAP, TextureBinder::NORMAL_MAP, &m_ShaderData.EnableNormalMap);
		BindTexture(MaterialTexture::ROUGHNESS_MAP, TextureBinder::ROUGHNESS_MAP, &m_ShaderData.EnableRoughnessMap);
		BindTexture(MaterialTexture::METALNESS_MAP, TextureBinder::METALNESS_MAP, &m_ShaderData.EnableMetalnessMap);
		BindTexture(MaterialTexture::AMBIENT_OCCLUSION_MAP, TextureBinder::AMBIENT_OCCLUSION_MAP, &m_ShaderData.EnableAmbientOcclusionMap);

		return m_ShaderData;
	}

	void Material::BindTexture(MaterialTexture textureType, TextureBinder binder, int* isEnabled)
	{
		if (m_TextureMap.find(textureType) != m_TextureMap.end())
		{
			*isEnabled = m_TextureMap.at(textureType).IsEnabled;
			if (*isEnabled)
			{
				m_TextureMap.at(textureType).Texture->Bind(binder);
			}
		}
		else
		{
			*isEnabled = false;
		}
	}
}
