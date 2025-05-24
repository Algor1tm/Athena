#pragma once

#include "Athena/Asset/Asset.h"
#include "Athena/Core/Core.h"


namespace Athena
{
	class AssetSerializer: public RefCounted
	{
	public:
		virtual void Serialize(const Ref<Asset>& asset, const AssetMetadata& metadata) = 0;
		virtual bool TryLoadData(const Ref<Asset>& asset, const AssetMetadata& metadata) = 0;
	};


	class MaterialSerializer: public AssetSerializer
	{
	public:
		virtual void Serialize(const Ref<Asset>& asset, const AssetMetadata& metadata) override;
		virtual bool TryLoadData(const Ref<Asset>& asset, const AssetMetadata& metadata) override;
	};


	class SceneAssetSerializer : public AssetSerializer
	{
	public:
		virtual void Serialize(const Ref<Asset>& asset, const AssetMetadata& metadata) override;
		virtual bool TryLoadData(const Ref<Asset>& asset, const AssetMetadata& metadata) override;
	};
	

	class StaticMeshSerializer : public AssetSerializer
	{
	public:
		virtual void Serialize(const Ref<Asset>& asset, const AssetMetadata& metadata) override;
		virtual bool TryLoadData(const Ref<Asset>& asset, const AssetMetadata& metadata) override;
	};


	class SkeletalMeshSerializer : public AssetSerializer
	{
	public:
		virtual void Serialize(const Ref<Asset>& asset, const AssetMetadata& metadata) override;
		virtual bool TryLoadData(const Ref<Asset>& asset, const AssetMetadata& metadata) override;
	};
}
