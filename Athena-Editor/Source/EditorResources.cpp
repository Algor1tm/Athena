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

		m_Icons["Titlebar_CloseWindow"] = Image::Create(m_Path / "Icons/Editor/Titlebar/CloseWindow.png");
		m_Icons["Titlebar_MinimizeWindow"] = Image::Create(m_Path / "Icons/Editor/Titlebar/MinimizeWindow.png");
		m_Icons["Titlebar_RestoreWindow"] = Image::Create(m_Path / "Icons/Editor/Titlebar/RestoreWindow.png");
		m_Icons["Titlebar_MaximizeWindow"] = Image::Create(m_Path / "Icons/Editor/Titlebar/MaximizeWindow.png");

		m_Icons["Viewport_Play"] = Image::Create(m_Path / "Icons/Editor/Viewport/Play.png");
		m_Icons["Viewport_Simulate"] = Image::Create(m_Path / "Icons/Editor/Viewport/Simulate.png");
		m_Icons["Viewport_Stop"] = Image::Create(m_Path / "Icons/Editor/Viewport/Stop.png");

		m_Icons["ContentBrowser_Folder"] = Image::Create(m_Path / "Icons/Editor/ContentBrowser/Folder.png");
		m_Icons["ContentBrowser_File"] = Image::Create(m_Path / "Icons/Editor/ContentBrowser/File.png");
		m_Icons["ContentBrowser_Undo"] = Image::Create(m_Path / "Icons/Editor/ContentBrowser/Undo.png");
		m_Icons["ContentBrowser_Redo"] = Image::Create(m_Path / "Icons/Editor/ContentBrowser/Redo.png");
		m_Icons["ContentBrowser_Refresh"] = Image::Create(m_Path / "Icons/Editor/ContentBrowser/Refresh.png");
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
