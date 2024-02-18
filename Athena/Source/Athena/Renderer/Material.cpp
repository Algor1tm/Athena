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
	}

	Material::~Material()
	{
		
	}

	void Material::Set(const String& name, const Matrix4& value)
	{
		Renderer::Submit([this, name, value]()
		{
			SetInternal(name, ShaderDataType::Mat4, &value);
		});
	}

	void Material::Set(const String& name, const Vector4& value)
	{
		Renderer::Submit([this, name, value]()
		{
			SetInternal(name, ShaderDataType::Float4, &value);
		});
	}

	void Material::Set(const String& name, float value)
	{
		Renderer::Submit([this, name, value]()
		{
			SetInternal(name, ShaderDataType::Float, &value);
		});
	}

	void Material::Set(const String& name, uint32 value)
	{
		Renderer::Submit([this, name, value]()
		{
			SetInternal(name, ShaderDataType::UInt, &value);
		});
	}

	bool Material::TryGetMemberData(const String& name, ShaderDataType dataType, StructMemberShaderMetaData* memberData)
	{
		if (!m_BufferMembers->contains(name))
		{
			ATN_CORE_WARN_TAG("Renderer", "Failed to get or set shader push constant member with name '{}' (invalid name)", name);
			return false;
		}

		*memberData = m_BufferMembers->at(name);
		if (memberData->Type != dataType)
		{
			ATN_CORE_WARN_TAG("Renderer", "Failed to get or set shader push constant member with name '{}' \
					(type is not matching: given - '{}', expected - '{}')", name, ShaderDataTypeToString(memberData->Type), ShaderDataTypeToString(dataType));
			return false;
		}

		return true;
	}

	void Material::SetInternal(const String& name, ShaderDataType dataType, const void* data)
	{
		StructMemberShaderMetaData memberData;
		if (TryGetMemberData(name, dataType, &memberData))
		{
			memcpy(&m_Buffer[memberData.Offset], data, memberData.Size);
		}
	}

	bool Material::GetInternal(const String& name, ShaderDataType dataType, void** data)
	{
		StructMemberShaderMetaData memberData;
		if (TryGetMemberData(name, dataType, &memberData))
		{
			*data = &m_Buffer[memberData.Offset];
			return true;
		}

		return false;
	}

	void Material::RT_UpdateForRendering(const Ref<RenderCommandBuffer>& commandBuffer)
	{
		RT_SetPushConstant(commandBuffer, m_Buffer);
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
