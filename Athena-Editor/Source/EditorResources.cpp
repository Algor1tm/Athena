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

		m_Icons["Titlebar_CloseWindow"] = Texture2D::Create(m_Path / "Icons/Editor/Titlebar/CloseWindow.png", true);
		m_Icons["Titlebar_MinimizeWindow"] = Texture2D::Create(m_Path / "Icons/Editor/Titlebar/MinimizeWindow.png", true);
		m_Icons["Titlebar_RestoreWindow"] = Texture2D::Create(m_Path / "Icons/Editor/Titlebar/RestoreWindow.png", true);
		m_Icons["Titlebar_MaximizeWindow"] = Texture2D::Create(m_Path / "Icons/Editor/Titlebar/MaximizeWindow.png", true);

		m_Icons["Viewport_Play"] = Texture2D::Create(m_Path / "Icons/Editor/Viewport/Play.png", true);
		m_Icons["Viewport_Simulate"] = Texture2D::Create(m_Path / "Icons/Editor/Viewport/Simulate.png", true);
		m_Icons["Viewport_Stop"] = Texture2D::Create(m_Path / "Icons/Editor/Viewport/Stop.png", true);
		m_Icons["Viewport_Camera"] = Texture2D::Create(m_Path / "Icons/Editor/Viewport/Camera.png", true);
		m_Icons["Viewport_PointLight"] = Texture2D::Create(m_Path / "Icons/Editor/Viewport/PointLight.png", true);
		m_Icons["Viewport_SpotLight"] = Texture2D::Create(m_Path / "Icons/Editor/Viewport/SpotLight.png", true);
		m_Icons["Viewport_DirLight"] = m_Icons["Viewport_SpotLight"];
		m_Icons["Viewport_SkyLight"] = Texture2D::Create(m_Path / "Icons/Editor/Viewport/SkyLight.png", true);

		m_Icons["ContentBrowser_Folder"] = Texture2D::Create(m_Path / "Icons/Editor/ContentBrowser/Folder.png", true);
		m_Icons["ContentBrowser_File"] = Texture2D::Create(m_Path / "Icons/Editor/ContentBrowser/File.png", true);
		m_Icons["ContentBrowser_Undo"] = Texture2D::Create(m_Path / "Icons/Editor/ContentBrowser/Undo.png", true);
		m_Icons["ContentBrowser_Redo"] = Texture2D::Create(m_Path / "Icons/Editor/ContentBrowser/Redo.png", true);
		m_Icons["ContentBrowser_Refresh"] = Texture2D::Create(m_Path / "Icons/Editor/ContentBrowser/Refresh.png", true);
	}

	void EditorResources::Shutdown()
	{
		m_Icons.clear();
	}

	const FilePath& EditorResources::GetPath()
	{
		return m_Path;
	}

	Ref<Texture2D> EditorResources::GetIcon(std::string_view name)
	{
		if (!m_Icons.contains(name) || m_Icons.at(name) == nullptr)
			return Renderer::GetWhiteTexture();

		return m_Icons.at(name);
	}
}
