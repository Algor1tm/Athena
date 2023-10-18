#include "Texture.h"

#include "Athena/Core/FileSystem.h"

#include "Athena/Renderer/Renderer.h"
#include "Athena/Platform/Vulkan/VulkanTexture2D.h"


namespace Athena
{
	Ref<Texture2D> Texture2D::Create(const FilePath& path)
	{
		return nullptr;
	}

	// From memory
	Ref<Texture2D> Texture2D::Create(const void* data, uint32 width, uint32 height)
	{
		return nullptr;
	}

	Ref<Texture2D> Texture2D::Create(const TextureCreateInfo& info)
	{
		switch (Renderer::GetAPI())
		{
		case Renderer::API::Vulkan: return Ref<VulkanTexture2D>::Create(info);
		case Renderer::API::None: return nullptr;
		}

		return nullptr;
	}

	Ref<TextureCube> TextureCube::Create(const TextureCreateInfo& info)
	{
		switch (Renderer::GetAPI())
		{
		case Renderer::API::Vulkan: return nullptr;
		case Renderer::API::None: return nullptr;
		}

		return nullptr;
	}

	Texture2DInstance::Texture2DInstance()
	{
		SetTexCoords({ Vector2{0.f, 0.f}, {1.f, 0.f}, {1.f, 1.f}, {0.f, 1.f} });
	}

	Texture2DInstance::Texture2DInstance(const Ref<Texture2D>& texture)
	{
		SetTexture(texture);
		SetTexCoords({ Vector2{0.f, 0.f}, {1.f, 0.f}, {1.f, 1.f}, {0.f, 1.f} });
	}

	Texture2DInstance::Texture2DInstance(const Ref<Texture2D>& texture, const std::array<Vector2, 4>& texCoords)
	{
		SetTexture(texture);
		SetTexCoords(texCoords);
	}

	Texture2DInstance::Texture2DInstance(const Ref<Texture2D>& texture, const Vector2& min, const Vector2& max)
	{
		SetTexture(texture);
		SetTexCoords(min, max);
	}

	void Texture2DInstance::SetTexCoords(const Vector2& min, const Vector2& max)
	{
		float width = (float)m_Texture->GetInfo().Width;
		float height = (float)m_Texture->GetInfo().Height;

		m_TexCoords[0] = { min.x / width, min.y / height };
		m_TexCoords[1] = { max.x / width, min.y / height };
		m_TexCoords[2] = { max.x / width, max.y / height };
		m_TexCoords[3] = { min.x / width, max.y / height };
	}
}
