#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Asset/Asset.h"
#include "Athena/Renderer/Texture.h"
#include "Athena/Renderer/ComputePass.h"
#include "Athena/Renderer/ComputePipeline.h"
#include "Athena/Renderer/Renderer.h"


namespace Athena
{
	enum class EnvironmentMapType
	{
		STATIC = 1,
		PREETHAM,
	};

	struct PreethamParams
	{
		bool operator==(const PreethamParams& other) const = default;

		float Resolution = 128.f;

		float Turbidity = 2.f;
		float Azimuth = 0.f;
		float Inclination = 0.f;
	};

	class ATHENA_API EnvironmentMap: public Asset
	{
	public:
		EnvironmentMap();

		Ref<TextureCube> GetEnvironmentTexture();
		Ref<TextureCube> GetIrradianceTexture();

		virtual AssetType GetAssetType() const override { return AssetType::EnvironmentMap; }

		virtual bool Serialize(const FilePath& absolutePath) const override;
		virtual bool Deserialize(const FilePath& absolutePath, Ref<AssetImportSettings> importSettings) override;

		static void CreatePreethamMap(const PreethamParams& params, Ref<TextureCube>& outEnvTex, Ref<TextureCube>& outIrradianceTex);
		static void ClearCache();

	private:
		static void CreateTextures(float resolution, Format floatFormat, Ref<TextureCube>& outEnvTex, Ref<TextureCube>& outIrradianceTex);
		static void FilterEnvironmentMap(const Ref<RenderCommandBuffer>& commandBuffer, Ref<TextureCube> envTex, Ref<TextureCube> irradianceTex);

	private:
		Ref<TextureCube> m_EnvironmentTexture;
		Ref<TextureCube> m_IrradianceTexture;
		uint32 m_Resolution = 1024;

		static const uint32 s_IrradianceMapResolution = 128;

		static Ref<TextureCube> s_CachedEnvMap;
		static Ref<TextureCube> s_CachedIrradiance;
		static PreethamParams s_CachedPreethamParams;
	};
}
