#include "SettingsPanel.h"

#include "Athena/Renderer/SceneRenderer.h"
#include "Athena/Renderer/Shader.h"

#include "Athena/Scripting/ScriptEngine.h"

#include "Athena/UI/UI.h"
#include "Athena/UI/Theme.h"

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

		ATN_ASSERT(false);
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

		ATN_ASSERT(false);
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

		ATN_ASSERT(false);
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

		ATN_ASSERT(false);
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

		if (UI::TreeNode("Debug", false))
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

		if (UI::TreeNode("LightEnvironmentSettings") && UI::BeginPropertyTable())
		{
			LightEnvironmentSettings& settings = SceneRenderer::GetSettings().LightEnvironmentSettings;

			UI::PropertyDrag("Exposure", &settings.Exposure);
			UI::PropertyDrag("Gamma", &settings.Gamma);

			UI::EndPropertyTable();
			UI::TreePop();
		}

		if (UI::TreeNode("Shadows") && UI::BeginPropertyTable())
		{
			ShadowSettings& settings = SceneRenderer::GetSettings().ShadowSettings;

			UI::PropertyCheckbox("Enable Shadows", &settings.EnableShadows);
			UI::PropertyCheckbox("Soft Shadows", &settings.SoftShadows);
			UI::PropertyDrag("Light Size", &settings.LightSize, 0.025f);
			UI::PropertyDrag("Max Distance", &settings.MaxDistance);
			UI::PropertyDrag("Fade Out", &settings.FadeOut);
			UI::PropertySlider("Split Factor", &settings.ExponentialSplitFactor, 0.f, 1.f);
			UI::PropertyDrag("NearPlaneOffset", &settings.NearPlaneOffset);
			UI::PropertyDrag("FarPlaneOffset", &settings.FarPlaneOffset);

			UI::EndPropertyTable();
			UI::TreePop();
		}

		if (UI::TreeNode("Bloom") && UI::BeginPropertyTable())
		{
			BloomSettings& settings = SceneRenderer::GetSettings().BloomSettings;

			UI::PropertyCheckbox("Enable Bloom", &settings.EnableBloom);
			UI::PropertyDrag("Intensity", &settings.Intensity, 0.1f, 0, 10);
			UI::PropertyDrag("Threshold", &settings.Threshold, 0.1f, 0, 10);
			UI::PropertyDrag("Knee", &settings.Knee, 0.05f, 0, 10);
			UI::PropertyDrag("DirtIntensity", &settings.DirtIntensity, 0.1f, 0, 200);

			if (UI::PropertyImage("Dirt Texture", settings.DirtTexture, { 45.f, 45.f }))
			{
				FilePath path = FileDialogs::OpenFile("DirtTexture (*png)\0*.png\0");
				if (!path.empty())
					settings.DirtTexture = Texture2D::Create(path);
			}

			UI::EndPropertyTable();
			UI::TreePop();
		}

		if (UI::TreeNode("Other") && UI::BeginPropertyTable())
		{
			SceneRendererSettings& settings = SceneRenderer::GetSettings();

			std::string_view views[] = { "None", "MSAA 2X (2D Only)", "MSAA 4X (2D Only)", "MSAA 8X (2D Only)"};
			std::string_view selected = AntialisingToString(settings.AntialisingMethod);

			if (UI::PropertyCombo("Antialiasing", views, std::size(views), &selected))
			{
				settings.AntialisingMethod = StringToAntialising(selected);
			}

			UI::PropertyRow("Reload Shaders", ImGui::GetFrameHeight());
			{
				ImGui::PushStyleColor(ImGuiCol_Button, UI::GetTheme().BackgroundDark);
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 10, 3 });
				if (ImGui::Button("Reload Shaders"))
				{
					Renderer::GetShaderLibrary()->Reload();
				}
				ImGui::PopStyleVar();
				ImGui::PopStyleColor();
			}

			UI::EndPropertyTable();
			UI::TreePop();
		}

		ImGui::End();

		ImGui::Begin("ScriptEngine");
		
		ImGui::Text("Reload Scripts On Start");
		ImGui::SameLine();
		ImGui::Checkbox("##OnStart", &m_EditorSettings.ReloadScriptsOnStart);

		ImGui::PushStyleColor(ImGuiCol_Button, UI::GetTheme().BackgroundDark);
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
