#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Renderer/Texture.h"


typedef unsigned int GLenum;

namespace Athena
{
	class ATHENA_API GLTextureCube : public TextureCube
	{
	public:
		GLTextureCube(const std::array<std::pair<TextureCubeTarget, FilePath>, 6>& faces, bool sRGB, const TextureSamplerCreateInfo& samplerInfo);
		GLTextureCube(TextureFormat format, uint32 width, uint32 height, const TextureSamplerCreateInfo& samplerInfo);

		~GLTextureCube();

		virtual void Bind(uint32 slot = 0) const override;
		virtual bool IsLoaded() const override;

		virtual void GenerateMipMap(uint32 maxLevel) override;
		virtual void BindAsImage(uint32 slot = 0, uint32 level = 0) override;

		virtual void SetFilters(TextureFilter min, TextureFilter mag) override;

	private:
		void CreateSampler(const TextureSamplerCreateInfo& info);

	private:
		GLenum m_GLRendererID;
		bool m_IsLoaded = false;
		GLenum m_InternalFormat;
		GLenum m_DataFormat;
	};
}
