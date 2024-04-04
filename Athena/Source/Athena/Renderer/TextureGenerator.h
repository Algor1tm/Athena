#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Texture.h"


namespace Athena
{
	class ATHENA_API TextureGenerator
	{
	public:
		static void Init();
		static void Shutdown();

		static Ref<Texture2D> GetWhiteTexture();
		static Ref<Texture2D> GetBlackTexture();
		static Ref<Texture2D> GetBRDF_LUT();
		static Ref<Texture2D> GetBlueNoise();
		static Ref<TextureCube> GetBlackTextureCube();
	};
}
