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
		static Ref<TextureCube> GetBlackTextureCube();

		static Ref<Texture2D> GetBRDF_LUT();
		static Ref<Texture2D> GetBlueNoise();

		static std::vector<Vector3> GetAOKernel(uint32 numSamples);
		static std::vector<Vector2> GetAOKernelNoise(uint32 numSamples);
	};
}
