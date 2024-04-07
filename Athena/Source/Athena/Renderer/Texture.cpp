#include "Texture.h"

#include "Athena/Core/FileSystem.h"
#include "Athena/Core/Log.h"
#include "Athena/Math/Common.h"
#include "Athena/Renderer/Renderer.h"
#include "Athena/Platform/Vulkan/VulkanTexture2D.h"
#include "Athena/Platform/Vulkan/VulkanTextureCube.h"
#include "Athena/Platform/Vulkan/VulkanTextureView.h"

#include <stb_image/stb_image.h>
#include <stb_image/stb_image_write.h>


namespace Athena
{
	namespace Utils
	{
		static void* ConvertRGBToRGBA(Vector<byte, 3>* data, uint32 width, uint32 height)
		{
			uint64 newSize = width * height * Texture::BytesPerPixel(TextureFormat::RGBA8);
			Vector<byte, 4>* newData = (Vector<byte, 4>*)malloc(newSize);

			for (uint64 i = 0; i < width * height; ++i)
			{
				newData[i][0] = data[i][0];
				newData[i][1] = data[i][1];
				newData[i][2] = data[i][2];
				newData[i][3] = 255;
			}

			stbi_image_free(data);
			return newData;
		}

		static void* ConvertRGB32FToRGBA32F(Vector3* data, uint32 width, uint32 height)
		{
			uint64 newSize = width * height * Texture::BytesPerPixel(TextureFormat::RGBA32F);
			Vector4* newData = (Vector4*)malloc(newSize);

			for (uint64 i = 0; i < width * height; ++i)
			{
				newData[i][0] = data[i][0];
				newData[i][1] = data[i][1];
				newData[i][2] = data[i][2];
				newData[i][3] = 1.f;
			}

			stbi_image_free(data);
			return newData;
		}

		static std::vector<char> ConvertPathToUTF8(const FilePath& path)
		{
			std::vector<char> utf8Path(path.u8string().size() + 1);
			stbiw_convert_wchar_to_utf8(utf8Path.data(), utf8Path.size(), path.c_str());
			return utf8Path;
		}
	}


	Vector2u Texture::GetMipSize(uint32 mip) const
	{
		uint32 width = GetWidth();
		uint32 height = GetHeight();

		Vector2u mipSize;
		mipSize.x = float(width) * Math::Pow<float>(0.5f, mip);
		mipSize.y = float(height) * Math::Pow<float>(0.5f, mip);

		return mipSize;
	}

	uint32 Texture::GetMipLevelsCount() const
	{
		return Math::Floor(Math::Log2(Math::Max<float>(GetWidth(), GetHeight()))) + 1;
	}

	Ref<TextureView> TextureView::Create(const Ref<Texture>& texture, const TextureViewCreateInfo& info)
	{
		switch (Renderer::GetAPI())
		{
		case Renderer::API::Vulkan: return Ref<VulkanTextureView>::Create(texture, info);
		case Renderer::API::None: return nullptr;
		}

		return nullptr;
	}

	Texture::~Texture()
	{
		ClearViews();
	}

	Ref<TextureView> Texture::GetMipView(uint32 mip)
	{
		TextureViewCreateInfo info;
		info.BaseMipLevel = mip;
		info.MipLevelCount = 1;

		if (GetType() == TextureType::TEXTURE_CUBE)
			info.LayerCount = 6;

		return GetView(info);
	}

	Ref<TextureView> Texture::GetLayerView(uint32 layer)
	{
		TextureViewCreateInfo info;

		if (GetType() == TextureType::TEXTURE_2D)
		{
			info.BaseLayer = layer;
			info.LayerCount = 1;
		}
		else
		{
			info.BaseLayer = layer * 6;
			info.LayerCount = 6;
		}

		return GetView(info);
	}

	Ref<TextureView> Texture::GetView(const TextureViewCreateInfo& info)
	{
		if (m_TextureViews.contains(info))
			return m_TextureViews.at(info);

		m_TextureViews[info] = TextureView::Create(Ref(this), info);
		return m_TextureViews.at(info);
	}

	void Texture::InvalidateViews()
	{
		// Remove mip views if they exceed max mip level
		std::vector<TextureViewCreateInfo> viewsToRemove;
		for (const auto& [info, _] : m_TextureViews)
		{
			if (info.BaseMipLevel + info.MipLevelCount > GetMipLevelsCount())
				viewsToRemove.push_back(info);
		}

		for (const auto& info : viewsToRemove)
		{
			m_TextureViews.erase(info);
		}

		for (auto& [info, view] : m_TextureViews)
		{
			view->Invalidate();
		}
	}

	void Texture::ClearViews()
	{
		m_TextureViews.clear();
	}


	Ref<Texture2D> Texture2D::Create(const TextureCreateInfo& info, Buffer data)
	{
		switch (Renderer::GetAPI())
		{
		case Renderer::API::Vulkan: return Ref<VulkanTexture2D>::Create(info, data);
		case Renderer::API::None: return nullptr;
		}

		return nullptr;
	}

	Ref<Texture2D> Texture2D::Create(const FilePath& filepath, bool sRGB, bool genMips)
	{
		ATN_CORE_VERIFY(FileSystem::Exists(filepath));

		int width, height, channels;
		bool HDR = false;
		TextureFormat format = TextureFormat::RGBA8;
		void* data = nullptr;

		auto utf8Path = Utils::ConvertPathToUTF8(filepath);
		if (stbi_is_hdr(utf8Path.data()))
		{
			data = stbi_loadf(utf8Path.data(), &width, &height, &channels, 0);
			HDR = true;
			switch (channels)
			{
			case 3: format = TextureFormat::RGB32F; break;
			case 4: format = TextureFormat::RGBA32F; break;
			default:
				ATN_CORE_ERROR("Failed to load image from {}, width = {}, height = {}, channels = {}", filepath, width, height, channels);
				ATN_CORE_ASSERT(false);
				return nullptr;
			}
		}
		else
		{
			data = stbi_load(utf8Path.data(), &width, &height, &channels, 0);
			switch (channels)
			{
			case 1: format = sRGB ? TextureFormat::R8_SRGB : TextureFormat::R8;	  break;
			case 2: format = sRGB ? TextureFormat::RG8_SRGB : TextureFormat::RG8;	  break;
			case 3: format = sRGB ? TextureFormat::RGB8_SRGB : TextureFormat::RGB8;  break;
			case 4: format = sRGB ? TextureFormat::RGBA8_SRGB : TextureFormat::RGBA8; break;
			default:
				ATN_CORE_ERROR("Failed to load image from {}, width = {}, height = {}, channels = {}", filepath, width, height, channels);
				ATN_CORE_ASSERT(false);
				return nullptr;
			}
		}
		ATN_CORE_ASSERT(data);

		if (format == TextureFormat::RGB8_SRGB || format == TextureFormat::RGB8)
		{
			data = Utils::ConvertRGBToRGBA((Vector<byte, 3>*)data, width, height);
			format = sRGB ? TextureFormat::RGBA8_SRGB : TextureFormat::RGBA8;
		}
		else if (format == TextureFormat::RGB32F)
		{
			data = Utils::ConvertRGB32FToRGBA32F((Vector3*)data, width, height);
			format = TextureFormat::RGBA32F;
		}

		Buffer buffer = Buffer::Copy(data, width * height * Texture::BytesPerPixel(format));

		TextureCreateInfo info;
		info.Name = filepath.filename().string();
		info.Format = format;
		info.Usage = TextureUsage::DEFAULT;
		info.Width = width;
		info.Height = height;
		info.Layers = 1;
		info.GenerateMipLevels = genMips;
		info.Sampler.MinFilter = TextureFilter::LINEAR;
		info.Sampler.MagFilter = TextureFilter::LINEAR;
		info.Sampler.MipMapFilter = TextureFilter::LINEAR;
		info.Sampler.Wrap = TextureWrap::REPEAT;

		Ref<Texture2D> result = Texture2D::Create(info, buffer);
		result->m_FilePath = filepath;

		stbi_image_free(data);
		buffer.Release();

		return result;
	}

	Ref<Texture2D> Texture2D::Create(const String& name, const void* inputData, uint32 inputWidth, uint32 inputHeight, bool sRGB, bool genMips)
	{
		int width, height, channels;
		void* data = nullptr;

		const uint32 size = inputHeight == 0 ? inputWidth : inputWidth * inputHeight;
		data = stbi_load_from_memory((const stbi_uc*)inputData, size, &width, &height, &channels, 0);
		ATN_CORE_ASSERT(data);

		TextureFormat format = TextureFormat::RGBA8_SRGB;

		switch (channels)
		{
		case 1: format = sRGB ? TextureFormat::R8_SRGB : TextureFormat::R8;    break;
		case 2: format = sRGB ? TextureFormat::RG8_SRGB : TextureFormat::RG8;   break;
		case 3: format = sRGB ? TextureFormat::RGB8_SRGB : TextureFormat::RGB8;  break;
		case 4: format = sRGB ? TextureFormat::RGBA8_SRGB : TextureFormat::RGBA8; break;
		default:
			ATN_CORE_ERROR("Failed to load image from memory, width = {}, height = {}, channels = {}", width, height, channels);
			ATN_CORE_ASSERT(false);
			return nullptr;
		}

		if (channels == 3)
		{
			data = Utils::ConvertRGBToRGBA((Vector<byte, 3>*)data, width, height);
			format = sRGB ? TextureFormat::RGBA8_SRGB : TextureFormat::RGBA8;
		}

		Buffer buffer = Buffer::Copy(data, width * height * Texture::BytesPerPixel(format));

		TextureCreateInfo info;
		info.Name = name;
		info.Format = format;
		info.Usage = TextureUsage::DEFAULT;
		info.Width = width;
		info.Height = height;
		info.Layers = 1;
		info.GenerateMipLevels = genMips;
		info.Sampler.MinFilter = TextureFilter::LINEAR;
		info.Sampler.MagFilter = TextureFilter::LINEAR;
		info.Sampler.MipMapFilter = TextureFilter::LINEAR;
		info.Sampler.Wrap = TextureWrap::REPEAT;

		Ref<Texture2D> result = Texture2D::Create(info, buffer);

		stbi_image_free(data);
		buffer.Release();

		return result;
	}

	void Texture2D::SaveContentToFile(const FilePath& path, Buffer imageData)
	{
		auto utf8Path = Utils::ConvertPathToUTF8(path);
		uint32 bpp = Texture::BytesPerPixel(m_Info.Format);
		stbi_write_png(utf8Path.data(), m_Info.Width, m_Info.Height, bpp, imageData.Data(), bpp * m_Info.Width);
	}

	Ref<TextureCube> TextureCube::Create(const TextureCreateInfo& info, Buffer data)
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
