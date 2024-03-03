#include "Material.h"

#include "Athena/Renderer/Renderer.h"
#include "Athena/Platform/Vulkan/VulkanMaterial.h"
#include "Athena/Math/Random.h"


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

	Ref<Material> Material::CreatePBRAnim(const String& name)
	{
		return Material::Create(Renderer::GetShaderPack()->Get("PBR_Anim"), name);
	}

	Material::Material(const Ref<Shader> shader, const String& name)
		: m_Shader(shader), m_Name(name), m_BufferMembers(&shader->GetMetaData().PushConstant.Members)
	{
		memset(m_Buffer, 0, sizeof(m_Buffer));

		m_Flags[MaterialFlag::CAST_SHADOWS] = true;
	}

	Material::~Material()
	{
		
	}

	void Material::Set(const String& name, const Matrix4& value)
	{
		uint32 offset;
		if (!GetMemberOffset(name, ShaderDataType::Mat4, &offset))
			return;

		Renderer::Submit([this, offset , value]()
		{
			memcpy(&m_Buffer[offset], &value, sizeof(value));
		});
	}

	void Material::Set(const String& name, const Vector2& value)
	{
		uint32 offset;
		if (!GetMemberOffset(name, ShaderDataType::Float2, &offset))
			return;

		Renderer::Submit([this, offset, value]()
		{
			memcpy(&m_Buffer[offset], &value, sizeof(value));
		});
	}

	void Material::Set(const String& name, const Vector4& value)
	{
		uint32 offset;
		if (!GetMemberOffset(name, ShaderDataType::Float4, &offset))
			return;

		Renderer::Submit([this, offset, value]()
		{
			memcpy(&m_Buffer[offset], &value, sizeof(value));
		});
	}

	void Material::Set(const String& name, float value)
	{
		uint32 offset;
		if (!GetMemberOffset(name, ShaderDataType::Float, &offset))
			return;

		Renderer::Submit([this, offset, value]()
		{
			memcpy(&m_Buffer[offset], &value, sizeof(value));
		});
	}

	void Material::Set(const String& name, uint32 value)
	{
		uint32 offset;
		if (!GetMemberOffset(name, ShaderDataType::UInt, &offset))
			return;

		Renderer::Submit([this, offset, value]()
		{
			memcpy(&m_Buffer[offset], &value, sizeof(value));
		});
	}

	void Material::Set(const String& name, int32 value)
	{
		uint32 offset;
		if (!GetMemberOffset(name, ShaderDataType::Int, &offset))
			return;

		Renderer::Submit([this, offset, value]()
		{
			memcpy(&m_Buffer[offset], &value, sizeof(value));
		});
	}

	bool Material::GetMemberOffset(const String& name, ShaderDataType dataType, uint32* offset)
	{
		if (!m_BufferMembers->contains(name))
		{
			ATN_CORE_WARN_TAG("Renderer", "Failed to get or set shader push constant member with name '{}' (invalid name)", name);
			return false;
		}

		const auto& data = m_BufferMembers->at(name);
		if (data.Type != dataType)
		{
			ATN_CORE_WARN_TAG("Renderer", "Failed to get or set shader push constant member with name '{}' \
					(type is not matching: given - '{}', expected - '{}')", name, ShaderDataTypeToString(dataType), ShaderDataTypeToString(data.Type));
			return false;
		}

		*offset = data.Offset;
		return true;
	}

	bool Material::GetInternal(const String& name, ShaderDataType dataType, void** data)
	{
		uint32 offset;
		if (GetMemberOffset(name, dataType, &offset))
		{
			*data = &m_Buffer[offset];
			return true;
		}

		return false;
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

	void MaterialTable::Invalidate()
	{
		std::vector<String> removeList;

		for (const auto& [name, material] : m_Materials)
		{
			if (material->GetCount() == 1)
				removeList.push_back(name);
		}

		for (const auto& name : removeList)
			m_Materials.erase(name);
	}
}
