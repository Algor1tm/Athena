#include "Texture.h"

#include "Athena/Core/FileSystem.h"

#include "Athena/Renderer/Renderer.h"
#include "Athena/Platform/Vulkan/VulkanTexture2D.h"

#include <stb_image/stb_image.h>


namespace Athena
{
	Ref<Texture2D> Texture2D::Create(const FilePath& filepath)
	{
		int width, height, channels;
		bool HDR = false;
		TextureFormat format = TextureFormat::RGBA8;
		void* data = nullptr;

		String path = filepath.string();
		if (stbi_is_hdr(path.data()))
		{
			data = stbi_loadf(path.data(), &width, &height, &channels, 0);
			HDR = true;
			switch (channels)
			{
			case 3: format = TextureFormat::RGB16F; break;
			case 4: format = TextureFormat::RGBA16F; break;
			default: ATN_CORE_ASSERT(false);
			}
		}
		else
		{
			data = stbi_load(path.data(), &width, &height, &channels, 0);
			switch (channels)
			{
			case 3: format = TextureFormat::RGB8; ATN_CORE_ASSERT(false); break;
			case 4: format = TextureFormat::RGBA8; break;
			default: ATN_CORE_ASSERT(false);
			}
		}
		ATN_CORE_ASSERT(data);

		TextureCreateInfo info;
		info.Data = data;
		info.Width = width;
		info.Height = height;
		info.Layers = 1;
		info.MipLevels = 1;
		info.GenerateMipMap = false;
		info.sRGB = HDR ? false : true;
		info.Format = format;
		info.Usage = TextureUsage::SHADER_READ_ONLY;
		info.GenerateSampler = true;
		info.SamplerInfo.MinFilter = TextureFilter::LINEAR;
		info.SamplerInfo.MagFilter = TextureFilter::LINEAR;
		info.SamplerInfo.MipMapFilter = TextureFilter::LINEAR;
		info.SamplerInfo.Wrap = TextureWrap::REPEAT;

		Ref<Texture2D> result = Texture2D::Create(info);
		result->m_FilePath = filepath;

		stbi_image_free(data);
		return result;
	}

	// From memory
	Ref<Texture2D> Texture2D::Create(const void* inputData, uint32 inputWidth, uint32 inputHeight)
	{
		int width, height, channels;
		void* data = nullptr;

		const uint32 size = inputHeight == 0 ? inputWidth : inputWidth * inputHeight;
		data = stbi_load_from_memory((const stbi_uc*)inputData, size, &width, &height, &channels, 0);
		ATN_CORE_ASSERT(data);

		TextureFormat format = TextureFormat::RGBA8;

		switch (channels)
		{
		case 3: format = TextureFormat::RGB8; ATN_CORE_ASSERT(false); break;
		case 4: format = TextureFormat::RGBA8; break;
		default: ATN_CORE_ASSERT(false);
		}

		TextureCreateInfo info;
		info.Data = data;
		info.Width = width;
		info.Height = height;
		info.Layers = 1;
		info.MipLevels = 1;
		info.GenerateMipMap = false;
		info.sRGB = true;
		info.Format = format;
		info.Usage = TextureUsage::SHADER_READ_ONLY;
		info.GenerateSampler = true;
		info.SamplerInfo.MinFilter = TextureFilter::LINEAR;
		info.SamplerInfo.MagFilter = TextureFilter::LINEAR;
		info.SamplerInfo.MipMapFilter = TextureFilter::LINEAR;
		info.SamplerInfo.Wrap = TextureWrap::REPEAT;

		Ref<Texture2D> result = Texture2D::Create(info);
		stbi_image_free(data);
		return result;
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
