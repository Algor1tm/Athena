#include "FileDialogs.h"
#include "Athena/Project/Project.h"

#include <portable-file-dialogs/portable-file-dialogs.h>


namespace Athena
{
	FilePath FileDialogs::OpenFile(const String& dialogName, const String& extlabel, const std::vector<String>& exts, const FilePath& defaultDir)
	{
		Ref<EditorAssetManager> assetManager = Project::GetActive() ? Project::GetEditorAssetManager() : nullptr;

		if (assetManager)
			assetManager->GetAssetWatcherThread().Suspend();

		std::vector<String> selection = pfd::open_file(dialogName, defaultDir.string(), GetFilters(extlabel, exts), false).result();

		if(assetManager)
			assetManager->GetAssetWatcherThread().Resume();

		if (!selection.empty())
			return selection[0];

		return "";
	}

	std::vector<FilePath> FileDialogs::OpenFiles(const String& dialogName, const String& extlabel, const std::vector<String>& exts, const FilePath& defaultDir)
	{
		Ref<EditorAssetManager> assetManager = Project::GetActive() ? Project::GetEditorAssetManager() : nullptr;

		if (assetManager)
			assetManager->GetAssetWatcherThread().Suspend();

		std::vector<String> selection = pfd::open_file(dialogName, defaultDir.string(), GetFilters(extlabel, exts), true).result();

		if (assetManager)
			assetManager->GetAssetWatcherThread().Resume();

		if (!selection.empty())
		{
			std::vector<FilePath> result;
			result.reserve(selection.size());

			for (const auto& item : selection)
				result.push_back(item);

			return result;
		}

		return {};
	}

	FilePath FileDialogs::OpenDirectory(const String& dialogName, const FilePath& startDir)
	{
		Ref<EditorAssetManager> assetManager = Project::GetActive() ? Project::GetEditorAssetManager() : nullptr;

		if (assetManager)
			assetManager->GetAssetWatcherThread().Suspend();

		String selection = pfd::select_folder(dialogName, startDir.string(), pfd::opt::none).result();
  
		if (assetManager)
			assetManager->GetAssetWatcherThread().Resume();

		return selection;
	}

	FilePath FileDialogs::SaveFile(const String& dialogName, const String& extlabel, const std::vector<String>& exts, const FilePath& defaultDir)
	{
		Ref<EditorAssetManager> assetManager = Project::GetActive() ? Project::GetEditorAssetManager() : nullptr;

		if (assetManager)
			assetManager->GetAssetWatcherThread().Suspend();

		String selection = pfd::save_file(dialogName, defaultDir.string(), GetFilters(extlabel, exts), true).result();

		if (assetManager)
			assetManager->GetAssetWatcherThread().Resume();

		return selection;
	}

	std::vector<String> FileDialogs::GetFilters(const String& label, const std::vector<String>& exts)
	{
		String extsString;

		for (const auto& ext: exts)
		{
			extsString += fmt::format("*{} ", ext);
		}

		// remove last space
		extsString.erase(extsString.end() - 1);

		return { label, extsString };
	}
}
