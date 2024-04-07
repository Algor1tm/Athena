#include "EditorResources.h"

#include "Athena/Asset/TextureImporter.h"
#include "Athena/Renderer/TextureGenerator.h"


namespace Athena
{
	FilePath EditorResources::m_Path;
	std::unordered_map<std::string_view, Ref<Texture2D>> EditorResources::m_Icons;


	void EditorResources::Init(const FilePath& path)
	{
		m_Path = path;

		TextureImportOptions options;
		options.sRGB = false;
		options.GenerateMipMaps = false;

		m_Icons["Logo"] = TextureImporter::Load(m_Path / "Icons/Logo/LogoWhite.png", options);
		m_Icons["EmptyTexture"] = TextureImporter::Load(m_Path / "Icons/Editor/Other/EmptyTexture.png", options);
		m_Icons["Viewport_Stop"] = TextureImporter::Load(m_Path / "Icons/Editor/Viewport/Stop.png", options);

		options.sRGB = true;

		m_Icons["Titlebar_CloseWindow"] = TextureImporter::Load(m_Path / "Icons/Editor/Titlebar/CloseWindow.png", options);
		m_Icons["Titlebar_MinimizeWindow"] = TextureImporter::Load(m_Path / "Icons/Editor/Titlebar/MinimizeWindow.png", options);
		m_Icons["Titlebar_RestoreWindow"] = TextureImporter::Load(m_Path / "Icons/Editor/Titlebar/RestoreWindow.png", options);
		m_Icons["Titlebar_MaximizeWindow"] = TextureImporter::Load(m_Path / "Icons/Editor/Titlebar/MaximizeWindow.png", options);

		m_Icons["Viewport_Play"] = TextureImporter::Load(m_Path / "Icons/Editor/Viewport/Play.png", options);
		m_Icons["Viewport_Simulate"] = TextureImporter::Load(m_Path / "Icons/Editor/Viewport/Simulate.png", options);
		m_Icons["Viewport_Camera"] = TextureImporter::Load(m_Path / "Icons/Editor/Viewport/Camera.png", options);
		m_Icons["Viewport_PointLight"] = TextureImporter::Load(m_Path / "Icons/Editor/Viewport/PointLight.png", options);
		m_Icons["Viewport_SpotLight"] = TextureImporter::Load(m_Path / "Icons/Editor/Viewport/SpotLight.png", options);
		m_Icons["Viewport_DirLight"] = m_Icons["Viewport_SpotLight"];
		m_Icons["Viewport_SkyLight"] = TextureImporter::Load(m_Path / "Icons/Editor/Viewport/SkyLight.png", options);

		m_Icons["ContentBrowser_Folder"] = TextureImporter::Load(m_Path / "Icons/Editor/ContentBrowser/Folder.png", options);
		m_Icons["ContentBrowser_File"] = TextureImporter::Load(m_Path / "Icons/Editor/ContentBrowser/File.png", options);
		m_Icons["ContentBrowser_Undo"] = TextureImporter::Load(m_Path / "Icons/Editor/ContentBrowser/Undo.png", options);
		m_Icons["ContentBrowser_Redo"] = TextureImporter::Load(m_Path / "Icons/Editor/ContentBrowser/Redo.png", options);
		m_Icons["ContentBrowser_Refresh"] = TextureImporter::Load(m_Path / "Icons/Editor/ContentBrowser/Refresh.png", options);
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
			return TextureGenerator::GetWhiteTexture();

		return m_Icons.at(name);
	}
}
