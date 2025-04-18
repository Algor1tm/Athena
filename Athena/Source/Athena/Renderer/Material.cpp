#include "Material.h"

#include "Athena/Renderer/Renderer.h"
#include "Athena/Platform/Vulkan/VulkanMaterial.h"
#include "Athena/Math/Random.h"


namespace Athena
{
	Ref<Material> Material::Create(const Ref<Shader>& shader, const String& name)
	{
		Ref<Material> material = Ref<VulkanMaterial>::Create(shader, name);

		switch (Renderer::GetAPI())
		{
		case Renderer::API::Vulkan: return material;
		case Renderer::API::None: return nullptr;
		}

		return nullptr;
	}

	Ref<Material> Material::CreatePBR(const String& name)
	{
		return Material::Create(Renderer::GetShaderPack()->Get("GBuffer_Anim"), name);
	}

	Ref<Material> Material::CreatePBR()
	{
		static uint32 counter = 0;

		String name = fmt::format("MaterialPBR.{:03}", counter++);

		// Shader for animated and static meshes has same layout, so we can use same shader for both
		return Material::Create(Renderer::GetShaderPack()->Get("GBuffer_Anim"), name);
	}

	Material::Material(const Ref<Shader> shader, const String& name)
		: m_Shader(shader), m_Name(name), m_BufferMembers(&shader->GetMetaData().PushConstant.Members)
	{
		memset(m_Buffer, 0, sizeof(m_Buffer));
		SetFlag(MaterialFlag::CastShadows);
	}

	Material::~Material()
	{
		
	}

	void Material::Set(const String& name, const Matrix4& value)
	{
		uint32 offset;
		if (!GetMemberOffset(name, ShaderDataType::Mat4, &offset))
			return;

		memcpy(&m_Buffer[offset], &value, sizeof(value));
	}

	void Material::Set(const String& name, const Vector2& value)
	{
		uint32 offset;
		if (!GetMemberOffset(name, ShaderDataType::Float2, &offset))
			return;

		memcpy(&m_Buffer[offset], &value, sizeof(value));
	}

	void Material::Set(const String& name, const Vector4& value)
	{
		uint32 offset;
		if (!GetMemberOffset(name, ShaderDataType::Float4, &offset))
			return;

		memcpy(&m_Buffer[offset], &value, sizeof(value));
	}

	void Material::Set(const String& name, float value)
	{
		uint32 offset;
		if (!GetMemberOffset(name, ShaderDataType::Float, &offset))
			return;

		memcpy(&m_Buffer[offset], &value, sizeof(value));
	}

	void Material::Set(const String& name, uint32 value)
	{
		uint32 offset;
		if (!GetMemberOffset(name, ShaderDataType::UInt, &offset))
			return;

		memcpy(&m_Buffer[offset], &value, sizeof(value));
	}

	void Material::Set(const String& name, int32 value)
	{
		uint32 offset;
		if (!GetMemberOffset(name, ShaderDataType::Int, &offset))
			return;

		memcpy(&m_Buffer[offset], &value, sizeof(value));
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

	bool Material::IsFlagSet(MaterialFlag flag) const
	{
		return m_BitField & uint32(flag);
	}

	void Material::SetFlag(MaterialFlag flag, bool value)
	{
		if (value)
			m_BitField = m_BitField | uint32(flag);
		else
			m_BitField = m_BitField & ~(uint32(flag));
	}
}
