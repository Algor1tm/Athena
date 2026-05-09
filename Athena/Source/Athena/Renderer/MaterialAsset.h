#pragma once

#include "Athena/Asset/Asset.h"
#include "Athena/Asset/AssetManager.h"
#include "Athena/Core/Core.h"
#include "Athena/Renderer/Material.h"


namespace Athena
{
	enum class MaterialTextureType
	{
		Albedo = 1,
		Normals,
		Roughness,
		Metalness
	};


	class ATHENA_API MaterialAsset: public Asset
	{
	public:
		MaterialAsset();

		static Ref<MaterialAsset> GetDefault();

		virtual AssetType GetAssetType() const override { return AssetType::Material; }

		virtual bool Serialize(const FilePath& absolutePath) const override;
		virtual bool Deserialize(const FilePath& absolutePath) override;

		Ref<Material> GetMaterial() const { return m_Material; }

		const LinearColor& GetAlbedo() const;
		void SetAlbedo(const LinearColor& albedo);

		float GetEmission() const;
		void SetEmission(float emission);

		float GetRoughness() const;
		void SetRoughness(float roughness);

		float GetMetalness() const;
		void SetMetalness(float metalness);

		AssetHandle GetTexture(MaterialTextureType type) const;
		void SetTexture(MaterialTextureType type, AssetHandle texture);
		void RemoveTexture(MaterialTextureType type);

		bool IsEnabledTexture(MaterialTextureType type) const;
		void EnableTexture(MaterialTextureType type, bool flag);

		bool IsFlagSet(MaterialFlag flag) const;
		void SetFlag(MaterialFlag flag, bool value = true);

		void UpdateTextureAssets();

	private:
		Ref<Material> m_Material;

		LinearColor m_Albedo = LinearColor(0.7f, 0.7f, 0.7f, 1.f);
		float m_Emission = 0.f;
		float m_Roughness = 0.8f;
		float m_Metalness = 0.f;
	
		std::unordered_map<MaterialTextureType, AssetHandle> m_TexturesMap;
		std::unordered_map<MaterialTextureType, bool> m_TexturesFlags;
	};
}
