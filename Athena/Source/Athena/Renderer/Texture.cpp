#include "Texture.h"

#include "Athena/Core/FileSystem.h"
#include "Athena/Core/Log.h"
#include "Athena/Math/Common.h"
#include "Athena/Renderer/Renderer.h"
#include "Athena/Platform/Vulkan/VulkanTexture2D.h"
#include "Athena/Platform/Vulkan/VulkanTextureCube.h"


namespace Athena
{
	Vector2u Texture::GetMipSize(uint32 mip) const
	{
		uint32 width = m_Image->GetInfo().Width;
		uint32 height = m_Image->GetInfo().Height;

		Vector2u mipSize;
		mipSize.x = float(width) * Math::Pow<float>(0.5f, mip);
		mipSize.y = float(height) * Math::Pow<float>(0.5f, mip);

		return mipSize;
	}

	Ref<Texture2D> Texture2D::Create(const Texture2DCreateInfo& info, Buffer data)
	{
		switch (Renderer::GetAPI())
		{
		case Renderer::API::Vulkan: return Ref<VulkanTexture2D>::Create(info, data);
		case Renderer::API::None: return nullptr;
		}

		return nullptr;
	}

	Ref<Texture2D> Texture2D::Create(const FilePath& path, bool sRGB, bool genMips)
	{
		Ref<Image> image = Image::Create(path, sRGB, genMips);
		TextureSamplerCreateInfo samplerInfo;
		samplerInfo.MinFilter = TextureFilter::LINEAR;
		samplerInfo.MagFilter = TextureFilter::LINEAR;
		samplerInfo.MipMapFilter = TextureFilter::LINEAR;
		samplerInfo.Wrap = TextureWrap::REPEAT;

		Ref<Texture2D> result = Texture2D::Create(image, samplerInfo);
		result->m_FilePath = path;

		return result;
	}

	Ref<Texture2D> Texture2D::Create(const String& name, const void* data, uint32 width, uint32 height, bool sRGB, bool genMips)
	{
		Ref<Image> image = Image::Create(name, data, width, height, sRGB, genMips);
		TextureSamplerCreateInfo samplerInfo;
		samplerInfo.MinFilter = TextureFilter::LINEAR;
		samplerInfo.MagFilter = TextureFilter::LINEAR;
		samplerInfo.MipMapFilter = TextureFilter::LINEAR;
		samplerInfo.Wrap = TextureWrap::REPEAT;

		Ref<Texture2D> result = Texture2D::Create(image, samplerInfo);
		return result;
	}

	Ref<Texture2D> Texture2D::Create(const Ref<Image>& image, const TextureSamplerCreateInfo& samplerInfo)
	{
		switch (Renderer::GetAPI())
		{
		case Renderer::API::Vulkan: return Ref<VulkanTexture2D>::Create(image, samplerInfo);
		case Renderer::API::None: return nullptr;
		}

		return nullptr;
	}

	Ref<TextureCube> TextureCube::Create(const TextureCubeCreateInfo& info, Buffer data)
	{
		switch (Renderer::GetAPI())
		{
		case Renderer::API::Vulkan: return Ref<VulkanTextureCube>::Create(info, data);
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
