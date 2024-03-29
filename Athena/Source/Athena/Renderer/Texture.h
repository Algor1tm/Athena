#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Math/Vector.h"
#include "Athena/Renderer/Color.h"
#include "Athena/Renderer/RenderResource.h"
#include "Athena/Renderer/Image.h"

#include <array>


namespace Athena
{
	enum class TextureFilter
	{
		LINEAR = 1,
		NEAREST = 2,
	};

	enum class TextureWrap
	{
		REPEAT = 1,
		CLAMP_TO_EDGE = 2,
		CLAMP_TO_BORDER = 3,
		MIRRORED_REPEAT = 4,
		MIRRORED_CLAMP_TO_EDGE = 5
	};

	enum class TextureCompareOperator
	{
		NONE = 0,
		LEQUAL = 1
	};

	struct TextureSamplerCreateInfo
	{
		// For hashing
		bool operator==(const TextureSamplerCreateInfo& other) const = default;

		TextureFilter MinFilter = TextureFilter::LINEAR;
		TextureFilter MagFilter = TextureFilter::LINEAR;
		TextureFilter MipMapFilter = TextureFilter::LINEAR;
		TextureWrap Wrap = TextureWrap::REPEAT;
		TextureCompareOperator Compare = TextureCompareOperator::NONE;
		// TODO: Anisotropy
	};

	class ATHENA_API Texture : public RenderResource
	{
	public:
		virtual ~Texture() = default;

		virtual void Resize(uint32 width, uint32 height) = 0;
		virtual void SetSampler(const TextureSamplerCreateInfo& samplerInfo) = 0;

		void BlitMipMap(uint32 levels) { m_Image->BlitMipMap(levels); }
		void BlitMipMap(const Ref<RenderCommandBuffer>& cmdBuffer, uint32 levels) { m_Image->BlitMipMap(cmdBuffer, levels); }
		uint32 GetMipLevelsCount() const { return m_Image->GetMipLevelsCount(); }
		Vector2u GetMipSize(uint32 mip) const;

		const Ref<Image>& GetImage() const { return m_Image; }

	protected:
		Ref<Image> m_Image;
	};


	struct Texture2DCreateInfo
	{
		String Name;
		ImageFormat Format = ImageFormat::RGBA8;
		ImageUsage Usage = ImageUsage::DEFAULT;
		const void* InitialData = nullptr;
		uint32 Width = 1;
		uint32 Height = 1;
		uint32 Layers = 1;
		bool GenerateMipLevels = false;
		TextureSamplerCreateInfo SamplerInfo;
	};

	class ATHENA_API Texture2D : public Texture
	{
	public:
		static Ref<Texture2D> Create(const Texture2DCreateInfo& info);
		static Ref<Texture2D> Create(const FilePath& path, bool sRGB = false, bool genMips = false);
		static Ref<Texture2D> Create(const String& name, const void* data, uint32 width, uint32 height, bool sRGB = false, bool genMips = false);
		static Ref<Texture2D> Create(const Ref<Image>& image, const TextureSamplerCreateInfo& samplerInfo);

		virtual RenderResourceType GetResourceType() const override { return RenderResourceType::Texture2D; }
		virtual const String& GetName() const override { return m_Info.Name; }

		uint32 GetWidth() const { return m_Info.Width; }
		uint32 GetHeight() const { return m_Info.Height; }
		Vector2u GetSize() const { return { m_Info.Width, m_Info.Height }; }
		ImageFormat GetFormat() const { return m_Info.Format; }
		const FilePath& GetFilePath() const { return m_FilePath; }

		const Texture2DCreateInfo& GetInfo() const { return m_Info; };

	protected:
		Texture2DCreateInfo m_Info;
		FilePath m_FilePath;
	};


	struct TextureCubeCreateInfo
	{
		String Name;
		ImageFormat Format = ImageFormat::RGBA8;
		ImageUsage Usage = ImageUsage::DEFAULT;
		const void* InitialData = nullptr;
		uint32 Width = 1;
		uint32 Height = 1;
		bool GenerateMipLevels = false;
		TextureSamplerCreateInfo SamplerInfo;
	};

	class ATHENA_API TextureCube: public Texture
	{
	public:
		static Ref<TextureCube> Create(const TextureCubeCreateInfo& info);

		virtual RenderResourceType GetResourceType() const override { return RenderResourceType::TextureCube; }
		virtual const String& GetName() const override { return m_Info.Name; }

		uint32 GetWidth() const { return m_Info.Width; }
		uint32 GetHeight() const { return m_Info.Height; }
		Vector2u GetSize() const { return { m_Info.Width, m_Info.Height }; }
		ImageFormat GetFormat() const { return m_Info.Format; }

		const TextureCubeCreateInfo& GetInfo() const { return m_Info; };

	protected:
		TextureCubeCreateInfo m_Info;
	};


	class ATHENA_API Texture2DInstance
	{
	public:
		Texture2DInstance();
		Texture2DInstance(const Ref<Texture2D>& texture);
		Texture2DInstance(const Ref<Texture2D>& texture, const std::array<Vector2, 4>& texCoords);
		Texture2DInstance(const Ref<Texture2D>& texture, const Vector2& min, const Vector2& max);

		inline const Ref<Texture2D>& GetNativeTexture() const { return m_Texture; }
		inline const std::array<Vector2, 4>& GetTexCoords() const { return m_TexCoords; };

		inline void SetTexture(const Ref<Texture2D>& texture) { m_Texture = texture; }
		inline void SetTexCoords(const std::array<Vector2, 4>& texCoords) { m_TexCoords = texCoords; }
		void SetTexCoords(const Vector2& min, const Vector2& max);

	private:
		Ref<Texture2D> m_Texture;
		std::array<Vector2, 4> m_TexCoords;
	};
}
