#pragma once 

#include "Athena/Core/Core.h"
#include "Athena/Asset/Asset.h"


namespace Athena
{
	class ATHENA_API AssetFileExtensions
	{
	public:
		static void Init();

		static std::vector<String> GetAssetExtensionsList(AssetType assetType);
		static AssetType ExtensionToAssetType(const FilePath& ext);

	private:
		static std::unordered_map<FilePath, AssetType> m_AssetFileExtensionMap;
	};
}
