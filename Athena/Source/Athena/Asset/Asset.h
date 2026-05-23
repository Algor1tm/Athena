#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/UUID.h"
#include "Athena/Asset/Editor/AssetImportSettings.h"


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
		Mesh,
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
		virtual bool Deserialize(const FilePath& absolutePath, Ref<AssetImportSettings> importSettings) { return true;  }

		AssetHandle Handle = 0;
	};
}
