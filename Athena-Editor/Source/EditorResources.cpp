#include "EditorResources.h"

#include "Athena/Asset/Editor/TextureImporter.h"
#include "Athena/Renderer/EngineTextures.h"


namespace Athena
{
	FilePath EditorResources::m_Path;
	std::unordered_map<std::string_view, Ref<Texture2D>> EditorResources::m_Icons;


	void EditorResources::Init(const FilePath& path)
	{
		m_Path = path;


		Ref<TextureImportSettings> settings = Ref<TextureImportSettings>::Create();
		settings->AnisotropyLevel = 0.f;
		settings->GenerateMipMaps = false;
		settings->FilterMode = TextureFilter::LINEAR;

		TextureImporter importer(settings);

		m_Icons["Logo"] = importer.Import(m_Path / "Icons/Logo/LogoWhite.png");
		m_Icons["EmptyTexture"] = importer.Import(m_Path / "Icons/Editor/Other/EmptyTexture.png");
		m_Icons["Settings"] = importer.Import(m_Path / "Icons/Editor/Other/Settings.png");
		m_Icons["Viewport_Stop"] = importer.Import(m_Path / "Icons/Editor/Viewport/Stop.png");

		settings->sRGB = true;

		m_Icons["Titlebar_CloseWindow"] = importer.Import(m_Path / "Icons/Editor/Titlebar/CloseWindow.png");
		m_Icons["Titlebar_MinimizeWindow"] = importer.Import(m_Path / "Icons/Editor/Titlebar/MinimizeWindow.png");
		m_Icons["Titlebar_RestoreWindow"] = importer.Import(m_Path / "Icons/Editor/Titlebar/RestoreWindow.png");
		m_Icons["Titlebar_MaximizeWindow"] = importer.Import(m_Path / "Icons/Editor/Titlebar/MaximizeWindow.png");

		m_Icons["Viewport_Play"] = importer.Import(m_Path / "Icons/Editor/Viewport/Play.png");
		m_Icons["Viewport_Simulate"] = importer.Import(m_Path / "Icons/Editor/Viewport/Simulate.png");
		m_Icons["Viewport_Camera"] = importer.Import(m_Path / "Icons/Editor/Viewport/Camera.png");
		m_Icons["Viewport_PointLight"] = importer.Import(m_Path / "Icons/Editor/Viewport/PointLight.png");
		m_Icons["Viewport_SpotLight"] = importer.Import(m_Path / "Icons/Editor/Viewport/SpotLight.png");
		m_Icons["Viewport_DirLight"] = m_Icons["Viewport_SpotLight"];
		m_Icons["Viewport_SkyLight"] = importer.Import(m_Path / "Icons/Editor/Viewport/SkyLight.png");

		m_Icons["ContentBrowser_Folder"] = importer.Import(m_Path / "Icons/Editor/ContentBrowser/Folder.png");
		m_Icons["ContentBrowser_File"] = importer.Import(m_Path / "Icons/Editor/ContentBrowser/File.png");
		m_Icons["ContentBrowser_Undo"] = importer.Import(m_Path / "Icons/Editor/ContentBrowser/Undo.png");
		m_Icons["ContentBrowser_Redo"] = importer.Import(m_Path / "Icons/Editor/ContentBrowser/Redo.png");
		m_Icons["ContentBrowser_Refresh"] = importer.Import(m_Path / "Icons/Editor/ContentBrowser/Refresh.png");
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
			return EngineTextures::GetWhiteTexture();

		return m_Icons.at(name);
	}
}
