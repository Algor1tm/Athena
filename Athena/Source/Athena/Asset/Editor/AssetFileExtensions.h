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
		static FilePath GetImportSettingsPath(const FilePath& assetPath);

	private:
		static std::unordered_map<FilePath, AssetType> m_AssetFileExtensionMap;
		static String m_ImportSettingsExt;
	};
}
