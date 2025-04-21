#include "FileDialogs.h"
#include "Athena/Project/Project.h"

#include <portable-file-dialogs/portable-file-dialogs.h>


namespace Athena
{
	FilePath FileDialogs::OpenFile(const String& dialogName, const std::vector<String>& filters, const FilePath& defaultDir)
	{
		Project::GetEditorAssetManager()->GetAssetThread().Pause();
		std::vector<String> selection = pfd::open_file(dialogName, defaultDir.string(), filters, false).result();
		Project::GetEditorAssetManager()->GetAssetThread().Resume();

		if (!selection.empty())
			return selection[0];

		return "";
	}

	std::vector<FilePath> FileDialogs::OpenFiles(const String& dialogName, const std::vector<String>& filters, const FilePath& defaultDir)
	{
		Project::GetEditorAssetManager()->GetAssetThread().Pause();
		std::vector<String> selection = pfd::open_file(dialogName, defaultDir.string(), filters, true).result();
		Project::GetEditorAssetManager()->GetAssetThread().Resume();

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
		Project::GetEditorAssetManager()->GetAssetThread().Pause();
		String selection = pfd::select_folder(dialogName, startDir.string(), pfd::opt::none).result();
		Project::GetEditorAssetManager()->GetAssetThread().Resume();

		return selection;
	}

	FilePath FileDialogs::SaveFile(const String& dialogName, const std::vector<String>& filters, const FilePath& defaultDir)
	{
		Project::GetEditorAssetManager()->GetAssetThread().Pause();
		String selection = pfd::save_file(dialogName, defaultDir.string(), filters, true).result();
		Project::GetEditorAssetManager()->GetAssetThread().Resume();

		return selection;
	}
}
