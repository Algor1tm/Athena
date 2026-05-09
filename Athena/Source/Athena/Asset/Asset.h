#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/UUID.h"


namespace Athena
{
	using AssetHandle = UUID;

	enum class AssetType
	{
		None = 0,
		Texture,
		EnvironmentMap,
		Material,
		Font,
		Scene,
		MeshSource,
		StaticMesh,
		SkeletalMesh
	};

	struct AssetMetadata
	{
		AssetType Type = AssetType::None;
		FilePath FilePath;
		bool IsMemoryOnly = false;
	};

	class ATHENA_API Asset: public RefCounted
	{
	public:
		virtual ~Asset() = default;
		virtual AssetType GetAssetType() const = 0;

		virtual bool Serialize(const FilePath& absolutePath) const { return true; }
		virtual bool Deserialize(const FilePath& absolutePath) { return true;  }

		virtual bool SerializeRuntime() const { return true; }
		virtual bool DeserializeRuntime() { return true; }

		AssetHandle Handle = 0;
	};
}
