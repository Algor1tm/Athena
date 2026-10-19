#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Texture.h"
#include "Athena/Renderer/Font.h"
#include "Athena/Renderer/MaterialAsset.h"


namespace Athena
{
	class ATHENA_API EngineRenderAssets
	{
	public:
		void Init();
		void Destroy();

		static EngineRenderAssets& Get()
		{
			static EngineRenderAssets s_Instance;
			return s_Instance;
		}

	public:
		Ref<Texture2D> WhiteTexture;
		Ref<Texture2D> BlackTexture;
		Ref<TextureCube> BlackTextureCube;

		Ref<Texture2D> BlueNoise;
		Ref<Texture2D> SMAA_AreaLUT;
		Ref<Texture2D> SMAA_SearchLUT;
		Ref<Texture2D> BRDF_LUT;

		Ref<TextureAsset> DefaultTextureAsset;
		Ref<MaterialAsset> DefaultMaterialAsset;
		Ref<Font> DefaultFontAsset;
	};

	class ATHENA_API Noise
	{
	public:
		static std::vector<Vector4> GetHBAOJitters(uint32 numSamples);
	};
}
