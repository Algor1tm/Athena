#include "Material.h"

#include "Athena/Renderer/Renderer.h"
#include "Athena/Platform/Vulkan/VulkanMaterial.h"


namespace Athena
{
	Ref<Material> Material::Create(const Ref<Shader>& shader, const String& name)
	{
		Ref<Material> material = Ref<VulkanMaterial>::Create(shader, name);
		Renderer::GetMaterialTable()->AddMaterial(material);

		switch (Renderer::GetAPI())
		{
		case Renderer::API::Vulkan: return material;
		case Renderer::API::None: return nullptr;
		}

		return nullptr;
	}

	Ref<Material> Material::CreatePBRStatic(const String& name)
	{
		return Material::Create(Renderer::GetShaderPack()->Get("Test"), name);
	}

	Material::~Material()
	{
		// 1 more in material table
		if (GetCount() == 2)
		{
			Renderer::GetMaterialTable()->RemoveMaterial(Ref(this));
		}
	}

	Ref<Material> MaterialTable::GetMaterial(const String& name) const
	{
		return m_Materials.at(name);
	}

	void MaterialTable::AddMaterial(const Ref<Material>& material)
	{
		m_Materials[material->GetName()] = material;
	}

	void MaterialTable::RemoveMaterial(const Ref<Material>& material)
	{
		m_Materials.erase(material->GetName());
	}

	bool MaterialTable::Exists(const String& name) const
	{
		return m_Materials.contains(name);
	}
}
