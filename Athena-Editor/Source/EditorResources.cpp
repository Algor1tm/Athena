#include "EditorResources.h"

#include "Athena/Renderer/Renderer.h"


namespace Athena
{
	FilePath EditorResources::m_Path;
	std::unordered_map<std::string_view, Ref<Image>> EditorResources::m_Icons;


	void EditorResources::Init(const FilePath& path)
	{
		m_Path = path;

		m_Icons["Logo"] = Image::Create(m_Path / "Icons/Logo/LogoWhite.png");

		m_Icons["Titlebar_CloseWindow"] = Image::Create(m_Path / "Icons/Editor/Titlebar/CloseWindow.png", true);
		m_Icons["Titlebar_MinimizeWindow"] = Image::Create(m_Path / "Icons/Editor/Titlebar/MinimizeWindow.png", true);
		m_Icons["Titlebar_RestoreWindow"] = Image::Create(m_Path / "Icons/Editor/Titlebar/RestoreWindow.png", true);
		m_Icons["Titlebar_MaximizeWindow"] = Image::Create(m_Path / "Icons/Editor/Titlebar/MaximizeWindow.png", true);

		m_Icons["Viewport_Play"] = Image::Create(m_Path / "Icons/Editor/Viewport/Play.png", true);
		m_Icons["Viewport_Simulate"] = Image::Create(m_Path / "Icons/Editor/Viewport/Simulate.png", true);
		m_Icons["Viewport_Stop"] = Image::Create(m_Path / "Icons/Editor/Viewport/Stop.png", true);

		m_Icons["ContentBrowser_Folder"] = Image::Create(m_Path / "Icons/Editor/ContentBrowser/Folder.png", true);
		m_Icons["ContentBrowser_File"] = Image::Create(m_Path / "Icons/Editor/ContentBrowser/File.png", true);
		m_Icons["ContentBrowser_Undo"] = Image::Create(m_Path / "Icons/Editor/ContentBrowser/Undo.png", true);
		m_Icons["ContentBrowser_Redo"] = Image::Create(m_Path / "Icons/Editor/ContentBrowser/Redo.png", true);
		m_Icons["ContentBrowser_Refresh"] = Image::Create(m_Path / "Icons/Editor/ContentBrowser/Refresh.png", true);
	}

	void EditorResources::Shutdown()
	{
		m_Icons.clear();
	}

	const FilePath& EditorResources::GetPath()
	{
		return m_Path;
	}

	Ref<Image> EditorResources::GetIcon(std::string_view name)
	{
		if (!m_Icons.contains(name) || m_Icons.at(name) == nullptr)
			return Renderer::GetWhiteTexture()->GetImage();

		return m_Icons.at(name);
	}
}
