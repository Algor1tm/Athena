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

	class ATHENA_API EnvironmentMap: public Asset
	{
	public:
		EnvironmentMap();

		Ref<TextureCube> GetEnvironmentTexture();
		Ref<TextureCube> GetIrradianceTexture();

		void SetResolution(uint32 resolution);

	private:
		void Load();
		virtual void LoadSourceTexture(const Ref<RenderCommandBuffer>& commandBuffer) = 0;

	protected:
		Ref<TextureCube> m_EnvironmentTexture;
		Ref<TextureCube> m_IrradianceTexture;

		bool m_Dirty = true;

		uint32 m_Resolution = 1024;
		const uint32 m_IrradianceMapResolution = 128;

		Ref<ComputePass> m_IrradiancePass;
		Ref<ComputePipeline> m_IrradiancePipeline;

		Ref<ComputePass> m_MipFilterPass;
		Ref<ComputePipeline> m_MipFilterPipeline;
		std::array<Ref<Material>, ShaderDef::MAX_SKYBOX_MAP_LOD> m_MipFilterMaterials;
	};

	class ATHENA_API StaticEnvironmentMap : public EnvironmentMap
	{
	public:
		StaticEnvironmentMap();

		const FilePath& GetFilePath() const { return m_FilePath; }
		virtual AssetType GetAssetType() const override { return AssetType::EnvironmentMap; }

		virtual bool Serialize(const FilePath& absolutePath) const override;
		virtual bool Deserialize(const FilePath& absolutePath, Ref<AssetImportSettings> importSettings) override;

	private:
		virtual void LoadSourceTexture(const Ref<RenderCommandBuffer>& commandBuffer) override;

	private:
		FilePath m_FilePath;

		Ref<ComputePass> m_PanoramaToCubePass;
		Ref<ComputePipeline> m_PanoramaToCubePipeline;
	};

	class ATHENA_API PreethamEnvironmentMap : public EnvironmentMap
	{
	public:
		PreethamEnvironmentMap();

		// TODO: Not technically asset, but for convenience keep this
		virtual AssetType GetAssetType() const override { return AssetType::EnvironmentMap; }

		void SetPreethamParams(float turbidity, float azimuth, float inclination);

		float GetTurbidity() const { return m_Turbidity; }
		float GetAzimuth() const { return m_Azimuth; }
		float GetInclination() const { return m_Inclination; }

	private:
		virtual void LoadSourceTexture(const Ref<RenderCommandBuffer>& commandBuffer) override;

	private:
		float m_Turbidity = 2.f;
		float m_Azimuth = 0.f;
		float m_Inclination = 0.f;

		Ref<ComputePass> m_PreethamPass;
		Ref<ComputePipeline> m_PreethamPipeline;
		Ref<Material> m_PreethamMaterial;
	};
}
