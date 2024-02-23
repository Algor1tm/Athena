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

		SceneRendererSettings& settings = m_ViewportRenderer->GetSettings();
		ShaderPack& shaderPack = *Renderer::GetShaderPack();

		bool shaderPackOpen = UI::TreeNode("ShaderPack", false);
		if (!shaderPackOpen)
		{
			float frameHeight = ImGui::GetFrameHeight();
			ImVec2 regionAvail = ImGui::GetContentRegionAvail();
			ImVec2 textSize = ImGui::CalcTextSize("Reload All");

			ImGui::SameLine(regionAvail.x - frameHeight);
			UI::ShiftCursor(-textSize.x, 0.f);
			if (ImGui::Button("Reload All"))
				shaderPack.Reload();

			UI::ShiftCursorY(-3.f);
		}

		if (shaderPackOpen)
		{
			if (UI::BeginPropertyTable())
			{
				for (const auto& [name, shader] : shaderPack)
				{
					bool isCompiled = shader->IsCompiled();

					if (!isCompiled)
						ImGui::PushStyleColor(ImGuiCol_Text, UI::GetTheme().ErrorText);

					UI::PropertyRow(name, ImGui::GetFrameHeight());

					if (!isCompiled)
						ImGui::PopStyleColor();

					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {10.f, 3.f});
					ImGui::PushID(name.c_str());

					if (UI::ButtonCentered("Reload"))
						shader->Reload();

					ImGui::PopID();
					ImGui::PopStyleVar();
				}

				UI::EndPropertyTable();
			}

			UI::TreePop();
		}

		if (UI::TreeNode("Debug", false))
		{
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
			LightEnvironmentSettings& lightSettings = settings.LightEnvironmentSettings;

			UI::PropertyDrag("Exposure", &lightSettings.Exposure, 0.05);
			UI::PropertyDrag("Gamma", &lightSettings.Gamma, 0.05);

			UI::EndPropertyTable();
			UI::TreePop();
		}

		if (UI::TreeNode("Shadows") && UI::BeginPropertyTable())
		{
			ShadowSettings& shadowSettings = settings.ShadowSettings;

			UI::PropertyCheckbox("Soft Shadows", &shadowSettings.SoftShadows);
			UI::PropertyDrag("Light Size", &shadowSettings.LightSize, 0.025f);
			UI::PropertyDrag("Max Distance", &shadowSettings.MaxDistance);
			UI::PropertyDrag("Fade Out", &shadowSettings.FadeOut);

			UI::EndPropertyTable();

			if (UI::TreeNode("Cascade Settings", true, true) && UI::BeginPropertyTable())
			{
				UI::PropertySlider("Split", &shadowSettings.CascadeSplit, 0.f, 1.f);
				UI::PropertyDrag("Blend Distance", &shadowSettings.CascadeBlendDistance);
				UI::PropertyDrag("NearPlaneOffset", &shadowSettings.NearPlaneOffset);
				UI::PropertyDrag("FarPlaneOffset", &shadowSettings.FarPlaneOffset);

				UI::EndPropertyTable();
				UI::TreePop();
			}

			if (UI::TreeNode("ShadowMap", false, true))
			{
				static int layer = 0;
				ImGui::SliderInt("Layer", &layer, 0, ShaderDef::SHADOW_CASCADES_COUNT - 1);

				Ref<Image> shadowMap = m_ViewportRenderer->GetShadowMap();
				ImGui::Image(UI::GetTextureLayerID(shadowMap, layer), { 256, 256 });

				UI::TreePop();
			}

			UI::TreePop();
		}

		if (UI::TreeNode("Bloom", false) && UI::BeginPropertyTable())
		{
			BloomSettings& bloomSettings = settings.BloomSettings;

			UI::PropertyCheckbox("Enable Bloom", &bloomSettings.EnableBloom);
			UI::PropertyDrag("Intensity", &bloomSettings.Intensity, 0.1f, 0, 10);
			UI::PropertyDrag("Threshold", &bloomSettings.Threshold, 0.1f, 0, 10);
			UI::PropertyDrag("Knee", &bloomSettings.Knee, 0.05f, 0, 10);
			UI::PropertyDrag("DirtIntensity", &bloomSettings.DirtIntensity, 0.1f, 0, 200);

			if (UI::PropertyImage("Dirt Texture", bloomSettings.DirtTexture, { 45.f, 45.f }))
			{
				FilePath path = FileDialogs::OpenFile("DirtTexture (*png)\0*.png\0");
				if (!path.empty())
					bloomSettings.DirtTexture = Texture2D::Create(path);
			}

			UI::EndPropertyTable();
			UI::TreePop();
		}

		if (UI::TreeNode("Other", false) && UI::BeginPropertyTable())
		{
			std::string_view views[] = { "None", "MSAA 2X (2D Only)", "MSAA 4X (2D Only)", "MSAA 8X (2D Only)"};
			std::string_view selected = AntialisingToString(settings.AntialisingMethod);

			if (UI::PropertyCombo("Antialiasing", views, std::size(views), &selected))
			{
				settings.AntialisingMethod = StringToAntialising(selected);
			}

			//UI::PropertyRow("Reload Shaders", ImGui::GetFrameHeight());
			//{
			//	ImGui::PushStyleColor(ImGuiCol_Button, UI::GetTheme().BackgroundDark);
			//	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 10, 3 });
			//	if (ImGui::Button("Reload Shaders"))
			//	{
			//		Renderer::GetShaderLibrary()->Reload();
			//	}
			//	ImGui::PopStyleVar();
			//	ImGui::PopStyleColor();
			//}

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
