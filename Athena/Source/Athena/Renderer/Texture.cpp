#include "Texture.h"

#include "Athena/Core/FileSystem.h"

#include "Athena/Renderer/Renderer.h"


namespace Athena
{
	Ref<Texture2D> Texture2D::Create(const FilePath& path, bool sRGB, const TextureSamplerCreateInfo& samplerInfo)
	{
		switch (Renderer::GetAPI())
		{
		case Renderer::API::Vulkan: return nullptr;
		case Renderer::API::None: return nullptr;
		}

		return nullptr;
	}

	Ref<Texture2D> Texture2D::Create(const void* data, uint32 width, uint32 height, bool sRGB, const TextureSamplerCreateInfo& samplerInfo)
	{
		switch (Renderer::GetAPI())
		{
		case Renderer::API::Vulkan: return nullptr;
		case Renderer::API::None: return nullptr;
		}

		return nullptr;
	}

	Ref<Texture2D> Texture2D::Create(TextureFormat format, uint32 width, uint32 height, const TextureSamplerCreateInfo& samplerInfo)
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
		float width = (float)m_Texture->GetWidth();
		float height = (float)m_Texture->GetHeight();

		m_TexCoords[0] = { min.x / width, min.y / height };
		m_TexCoords[1] = { max.x / width, min.y / height };
		m_TexCoords[2] = { max.x / width, max.y / height };
		m_TexCoords[3] = { min.x / width, max.y / height };
	}

	Ref<TextureCube> TextureCube::Create(const std::array<std::pair<TextureCubeTarget, FilePath>, 6>& faces, bool sRGB, const TextureSamplerCreateInfo& samplerInfo)
	{
		switch (Renderer::GetAPI())
		{
		case Renderer::API::Vulkan: return nullptr;
		case Renderer::API::None: return nullptr;
		}

		return nullptr;
	}

	Ref<TextureCube> TextureCube::Create(TextureFormat format, uint32 width, uint32 height, const TextureSamplerCreateInfo& samplerInfo)
	{
		switch (Renderer::GetAPI())
		{
		case Renderer::API::Vulkan: return nullptr;
		case Renderer::API::None: return nullptr;
		}

		return nullptr;
	}

	Ref<TextureSampler> TextureSampler::Create(const TextureSamplerCreateInfo& info)
	{
		switch (Renderer::GetAPI())
		{
		case Renderer::API::Vulkan: return nullptr;
		case Renderer::API::None: return nullptr;
		}

		return nullptr;
	}
}
