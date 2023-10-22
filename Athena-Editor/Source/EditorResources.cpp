#include "EditorResources.h"

#include "Athena/Renderer/Renderer.h"


namespace Athena
{
	FilePath EditorResources::m_Path;
	std::unordered_map<std::string_view, Ref<Texture2D>> EditorResources::m_Icons;


	void EditorResources::Init(const FilePath& path)
	{
		m_Path = path;

		m_Icons["Logo"] = Texture2D::Create(m_Path / "Icons/Logo/LogoWhite.png");

		m_Icons["Titlebar_CloseWindow"] = Texture2D::Create(m_Path / "Icons/Editor/Titlebar/CloseWindow.png");
		m_Icons["Titlebar_MinimizeWindow"] = Texture2D::Create(m_Path / "Icons/Editor/Titlebar/MinimizeWindow.png");
		m_Icons["Titlebar_RestoreWindow"] = Texture2D::Create(m_Path / "Icons/Editor/Titlebar/RestoreWindow.png");
		m_Icons["Titlebar_MaximizeWindow"] = Texture2D::Create(m_Path / "Icons/Editor/Titlebar/MaximizeWindow.png");

		m_Icons["Viewport_Play"] = Texture2D::Create(m_Path / "Icons/Editor/Viewport/Play.png");
		m_Icons["Viewport_Simulate"] = Texture2D::Create(m_Path / "Icons/Editor/Viewport/Simulate.png");
		m_Icons["Viewport_Stop"] = Texture2D::Create(m_Path / "Icons/Editor/Viewport/Stop.png");

		m_Icons["ContentBrowser_Folder"] = Texture2D::Create(m_Path / "Icons/Editor/ContentBrowser/Folder.png");
		m_Icons["ContentBrowser_File"] = Texture2D::Create(m_Path / "Icons/Editor/ContentBrowser/File.png");
		m_Icons["ContentBrowser_Undo"] = Texture2D::Create(m_Path / "Icons/Editor/ContentBrowser/Undo.png");
		m_Icons["ContentBrowser_Redo"] = Texture2D::Create(m_Path / "Icons/Editor/ContentBrowser/Redo.png");
		m_Icons["ContentBrowser_Refresh"] = Texture2D::Create(m_Path / "Icons/Editor/ContentBrowser/Refresh.png");
	}

	void EditorResources::Shutdown()
	{
		m_Icons["Logo"].Release();

		m_Icons["Titlebar_CloseWindow"].Release();
		m_Icons["Titlebar_MinimizeWindow"].Release();
		m_Icons["Titlebar_RestoreWindow"].Release();
		m_Icons["Titlebar_MaximizeWindow"].Release();

		m_Icons["Viewport_Play"].Release();
		m_Icons["Viewport_Simulate"].Release();
		m_Icons["Viewport_Stop"].Release();

		m_Icons["ContentBrowser_Folder"].Release();
		m_Icons["ContentBrowser_File"].Release();
		m_Icons["ContentBrowser_Undo"].Release();
		m_Icons["ContentBrowser_Redo"].Release();
		m_Icons["ContentBrowser_Refresh"].Release();
	}

	const FilePath& EditorResources::GetPath()
	{
		return m_Path;
	}

	Ref<Texture2D> EditorResources::GetIcon(std::string_view name)
	{
		if (m_Icons.find(name) == m_Icons.end())
		{
			ATN_WARN_TAG("EditorResources", "Failed to get icon with name '{}'", name);
			return Renderer::GetWhiteTexture();
		}

		return m_Icons.at(name);
	}
}
