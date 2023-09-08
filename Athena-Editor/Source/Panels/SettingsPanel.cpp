#include "SettingsPanel.h"

#include "Athena/Renderer/SceneRenderer.h"
#include "Athena/Renderer/Shader.h"

#include "Athena/Scripting/ScriptEngine.h"

#include "Athena/UI/Widgets.h"

#include "EditorLayer.h"

#include <ImGui/imgui.h>


namespace Athena
{
	static std::string_view AntialisingToString(Antialising antialiasing)
	{
		switch (antialiasing)
		{
		case Antialising::NONE: return "None";
		case Antialising::MSAA_2X: return "MSAA 2X (2D Only)";
		case Antialising::MSAA_4X: return "MSAA 4X (2D Only)";
		case Antialising::MSAA_8X: return "MSAA 8X (2D Only)";
		}

		ATN_CORE_ASSERT(false);
		return "";
	}

	static Antialising StringToAntialising(std::string_view str)
	{
		if(str == "None")
			return Antialising::NONE;

		if(str == "MSAA 2X (2D Only)")
			return Antialising::MSAA_2X;

		if (str == "MSAA 4X (2D Only)")
			return Antialising::MSAA_4X;

		if (str == "MSAA 8X (2D Only)")
			return Antialising::MSAA_8X;

		ATN_CORE_ASSERT(false);
		return (Antialising)0;
	}

	static std::string_view DebugViewToString(DebugView view)
	{
		switch (view)
		{
		case DebugView::NONE: return "None";
		case DebugView::WIREFRAME: return "Wireframe";
		case DebugView::SHADOW_CASCADES: return "ShadowCascades";
		}

		ATN_CORE_ASSERT(false);
		return "";
	}

	static DebugView StringToDebugView(std::string_view str)
	{
		if (str == "None")
			return DebugView::NONE;

		if (str == "Wireframe")
			return DebugView::WIREFRAME;

		if (str == "ShadowCascades")
			return DebugView::SHADOW_CASCADES;

		ATN_CORE_ASSERT(false);
		return (DebugView)0;
	}

	SettingsPanel::SettingsPanel(std::string_view name, const Ref<EditorContext>& context)
		: Panel(name, context)
	{

	}

	void SettingsPanel::OnImGuiRender()
	{

		if (ImGui::Begin("Editor Settings"))
		{
			UI::ShiftCursorY(2.f);
			ImGui::Text("Show Physics Colliders"); ImGui::SameLine(); ImGui::Checkbox("##Show Physics Colliders", &m_EditorSettings.ShowPhysicsColliders);

			ImGui::Text("Camera Speed");
			ImGui::SameLine();
			ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
			ImGui::SliderFloat("##CameraSpeed", &m_EditorSettings.CameraSpeedLevel, 0.f, 10.f);
		}

		ImGui::End();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0.f });
		ImGui::Begin("SceneRenderer");
		ImGui::PopStyleVar();

		if (UI::TreeNode("Debug"))
		{
			auto stats = Renderer::GetStatistics();

			ImGui::Text("Draw Calls: %d", stats.DrawCalls);
			ImGui::Text("Dispatch Calls: %d", stats.DispatchCalls);
			ImGui::Text("Shaders Binded: %d", stats.ShadersBinded);
			ImGui::Text("Pipelines Binded: %d", stats.PipelinesBinded);
			ImGui::Text("Render Passes: %d", stats.RenderPasses);
			ImGui::Text("Compute Passes: %d", stats.ComputePasses);

			ImGui::Spacing();
			ImGui::Spacing();

			SceneRendererSettings& settings = SceneRenderer::GetSettings();

			ImGui::Text("DebugView");
			ImGui::SameLine();

			std::string_view views[] = { "None", "Wireframe", "ShadowCascades" };
			std::string_view selected = DebugViewToString(settings.DebugView);
			if (UI::ComboBox("##DebugView", views, std::size(views), &selected))
			{
				settings.DebugView = StringToDebugView(selected);
			}

			ImGui::Spacing();
			ImGui::Spacing();

			UI::TreePop();
		}

		if (UI::TreeNode("Shadows", false))
		{
			SceneRendererSettings& settings = SceneRenderer::GetSettings();

			ImGui::Text("Enable Shadows"); ImGui::SameLine(); ImGui::Checkbox("##Enable Shadows", &settings.ShadowSettings.EnableShadows);
			ImGui::Text("Soft Shadows"); ImGui::SameLine(); ImGui::Checkbox("##Soft Shadows", &settings.ShadowSettings.SoftShadows);
			ImGui::Text("Light Size"); ImGui::SameLine(); ImGui::DragFloat("##Light Size", &settings.ShadowSettings.LightSize, 0.025f);
			ImGui::Text("Max Distance"); ImGui::SameLine(); ImGui::DragFloat("##Max Distance", &settings.ShadowSettings.MaxDistance);
			ImGui::Text("Fade Out"); ImGui::SameLine(); ImGui::DragFloat("##Fade Out", &settings.ShadowSettings.FadeOut);
			ImGui::Text("Split Factor"); ImGui::SameLine(); ImGui::SliderFloat("##Split Factor", &settings.ShadowSettings.ExponentialSplitFactor, 0.f, 1.f);
			ImGui::Text("NearPlaneOffset"); ImGui::SameLine(); ImGui::DragFloat("##NearPlaneOffset", &settings.ShadowSettings.NearPlaneOffset);
			ImGui::Text("FarPlaneOffset"); ImGui::SameLine(); ImGui::DragFloat("##FarPlaneOffset", &settings.ShadowSettings.FarPlaneOffset);

			UI::TreePop();
		}

		if (UI::TreeNode("Bloom"))
		{
			SceneRendererSettings& settings = SceneRenderer::GetSettings();

			ImGui::Text("Enable Bloom"); ImGui::SameLine(); ImGui::Checkbox("##Enable Shadows", &settings.BloomSettings.EnableBloom);
			ImGui::Text("Intensity"); ImGui::SameLine(); ImGui::DragFloat("##Intensity", &settings.BloomSettings.Intensity, 0.1f, 0, 10);
			ImGui::Text("Threshold"); ImGui::SameLine(); ImGui::DragFloat("##Threshold", &settings.BloomSettings.Threshold, 0.1f, 0, 10);
			ImGui::Text("Knee"); ImGui::SameLine(); ImGui::DragFloat("##Knee", &settings.BloomSettings.Knee, 0.05f, 0, 10);
			ImGui::Text("DirtIntensity"); ImGui::SameLine(); ImGui::DragFloat("##DirtIntensity", &settings.BloomSettings.DirtIntensity, 0.1f, 0, 200);

			ImGui::Text("Dirt Texture"); 
			ImGui::SameLine();
			auto dirtTexture = settings.BloomSettings.DirtTexture ? settings.BloomSettings.DirtTexture : Renderer::GetWhiteTexture();
			if (ImGui::ImageButton(dirtTexture->GetRendererID(), { 50, 50 }))
			{
				FilePath path = FileDialogs::OpenFile("DirtTexture (*png)\0*.png\0");
				if (!path.empty())
					settings.BloomSettings.DirtTexture = Texture2D::Create(path);
			}

			UI::TreePop();
		}

		if (UI::TreeNode("Other", false))
		{
			SceneRendererSettings& settings = SceneRenderer::GetSettings();

			ImGui::Text("Antialiasing");
			ImGui::SameLine();

			std::string_view views[] = { "None", "MSAA 2X (2D Only)", "MSAA 4X (2D Only)", "MSAA 8X (2D Only)"};
			std::string_view selected = AntialisingToString(settings.AntialisingMethod);
			if (UI::ComboBox("##Antialiasing", views, std::size(views), &selected))
			{
				settings.AntialisingMethod = StringToAntialising(selected);
			}

			ImGui::Text("Reload Shaders");
			ImGui::SameLine();

			ImGui::PushStyleColor(ImGuiCol_Button, UI::GetDarkColor());
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 10, 3 });
			if (ImGui::Button("Reload Shaders"))
			{
				Renderer::GetShaderLibrary()->Reload();
			}
			ImGui::PopStyleVar();
			ImGui::PopStyleColor();

			UI::TreePop();
		}

		ImGui::End();

		ImGui::Begin("ScriptEngine");
		
		ImGui::Text("Reload Scripts On Start");
		ImGui::SameLine();
		ImGui::Checkbox("##OnStart", &m_EditorSettings.ReloadScriptsOnStart);

		ImGui::PushStyleColor(ImGuiCol_Button, UI::GetDarkColor());
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 10, 3 });
		if (ImGui::Button("Reload All Scripts"))
		{
			m_EditorCtx.ActiveScene->LoadAllScripts();
		}
		ImGui::PopStyleVar();
		ImGui::PopStyleColor();


		ImGui::End();
	}
}
