#include "Material.h"

#include "Athena/Renderer/Renderer.h"
#include "Athena/Platform/Vulkan/VulkanMaterial.h"


namespace Athena
{
	Ref<Material> Material::Create(const Ref<Shader>& shader, const String& name)
	{
		Ref<Material> material = Ref<VulkanMaterial>::Create(shader, name);
		Renderer::GetMaterialTable()->Add(material);

		switch (Renderer::GetAPI())
		{
		case Renderer::API::Vulkan: return material;
		case Renderer::API::None: return nullptr;
		}

		return nullptr;
	}

	Ref<Material> Material::CreatePBRStatic(const String& name)
	{
		return Material::Create(Renderer::GetShaderPack()->Get("PBR_Static"), name);
	}

	Material::~Material()
	{
		// 1 more in material table
		if (GetCount() == 2)
		{
			Renderer::GetMaterialTable()->Remove(Ref(this));
		}
	}

	void Material::Set(const String& name, const Matrix4& value)
	{
		SetInternal(name, ShaderDataType::Mat4, &value);
	}

	void Material::Set(const String& name, const Vector4& value)
	{
		SetInternal(name, ShaderDataType::Float4, &value);
	}

	void Material::Set(const String& name, float value)
	{
		SetInternal(name, ShaderDataType::Float, &value);
	}

	void Material::Set(const String& name, uint32 value)
	{
		SetInternal(name, ShaderDataType::UInt, &value);
	}


	Ref<Material> MaterialTable::Get(const String& name) const
	{
		return m_Materials.at(name);
	}

	void MaterialTable::Add(const Ref<Material>& material)
	{
		m_Materials[material->GetName()] = material;
	}

	void MaterialTable::Remove(const Ref<Material>& material)
	{
		m_Materials.erase(material->GetName());
	}

	bool MaterialTable::Exists(const String& name) const
	{
		return m_Materials.contains(name);
	}
}
