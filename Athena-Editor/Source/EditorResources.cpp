#include "EditorResources.h"


namespace Athena
{
	FilePath EditorResources::m_Path;
	std::unordered_map<std::string_view, Ref<Texture2D>> EditorResources::m_Icons;


	void EditorResources::Init(const FilePath& path)
	{
		m_Path = path;

		m_Icons["Logo"] = Texture2D::Create(m_Path / "Icons/Editor/MenuBar/LogoWhite.png");

		m_Icons["Titlebar_Close"] = Texture2D::Create(m_Path / "Icons/Editor/MenuBar/CloseButton.png");
		m_Icons["Titlebar_Minimize"] = Texture2D::Create(m_Path / "Icons/Editor/MenuBar/MinimizeButton.png");
		m_Icons["Titlebar_Restore"] = Texture2D::Create(m_Path / "Icons/Editor/MenuBar/RestoreButton.png");
		m_Icons["Titlebar_Maximize"] = Texture2D::Create(m_Path / "Icons/Editor/MenuBar/MaximizeButton.png");

		m_Icons["Viewport_Play"] = Texture2D::Create(m_Path / "Icons/Editor/MenuBar/PlayIcon.png");
		m_Icons["Viewport_Simulate"] = Texture2D::Create(m_Path / "Icons/Editor/MenuBar/SimulationIcon.png");
		m_Icons["Viewport_Stop"] = Texture2D::Create(m_Path / "Icons/Editor/MenuBar/StopIcon.png");

		m_Icons["ContentBrowser_Folder"] = Texture2D::Create(m_Path / "Icons/Editor/ContentBrowser/FolderIcon.png");
		m_Icons["ContentBrowser_File"] = Texture2D::Create(m_Path / "Icons/Editor/ContentBrowser/FileIcon.png");
		m_Icons["ContentBrowser_Undo"] = Texture2D::Create(m_Path / "Icons/Editor/ContentBrowser/BackButton.png");
		m_Icons["ContentBrowser_Redo"] = Texture2D::Create(m_Path / "Icons/Editor/ContentBrowser/ForwardButton.png");
	}

	const FilePath& EditorResources::GetPath()
	{
		return m_Path;
	}

	Ref<Texture2D> EditorResources::GetIcon(std::string_view name)
	{
		return m_Icons.at(name);
	}
}
