#include "SettingsPanel.h"

#include "Athena/Renderer/SceneRenderer.h"
#include "Athena/Renderer/Shader.h"

#include "Athena/Scripting/PublicScriptEngine.h"

#include "UI/Widgets.h"

#include <ImGui/imgui.h>


namespace Athena
{
	static std::string_view AntialisingToString(Antialising antialiasing)
	{
		switch (antialiasing)
		{
		case Antialising::NONE: return "None";
		case Antialising::MSAA_2X: return "MSAA 2X";
		case Antialising::MSAA_4X: return "MSAA 4X";
		case Antialising::MSAA_8X: return "MSAA 8X";
		}

		ATN_CORE_ASSERT(false);
		return "";
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

	SettingsPanel::SettingsPanel(std::string_view name)
		: Panel(name)
	{

	}

	void SettingsPanel::OnImGuiRender()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0.f });
		if (ImGui::Begin("Editor Settings"))
		{
			if (UI::BeginTreeNode("Physics"))
			{
				UI::ShiftCursorY(2.f);
				UI::DrawImGuiWidget("Show Physics Colliders", [this]() { return ImGui::Checkbox("##Show Physics Colliders", &m_EditorSettings.m_ShowPhysicsColliders); });
				UI::EndTreeNode();
			}

			if (UI::BeginTreeNode("Scripting"))
			{
				UI::ShiftCursorY(2.f);
				ImGui::PushStyleColor(ImGuiCol_Button, UI::GetDarkColor());
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 10, 3 });
				if (ImGui::Button("Reload Scripts"))
				{
					PublicScriptEngine::ReloadScripts();
				}
				ImGui::PopStyleVar();
				ImGui::PopStyleColor();

				UI::EndTreeNode();
			}

			if (UI::BeginTreeNode("Renderer"))
			{
				ImGui::Text("Camera Speed");
				ImGui::SameLine();
				ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
				ImGui::SliderFloat("##CameraSpeed", &m_EditorSettings.m_CameraSpeedLevel, 0.f, 10.f);

				UI::EndTreeNode();
			}
		}

		ImGui::End();


		ImGui::Begin("SceneRenderer");
		ImGui::PopStyleVar();

		if (UI::BeginTreeNode("Debug"))
		{
			auto stats = SceneRenderer::GetStatistics();

			ImGui::Text("Draw Calls: %d", stats.DrawCalls);
			ImGui::Text("Directional Lights: %d", stats.DirectionalLightsCount);
			ImGui::Text("Point Lights: %d", stats.PointLightsCount);

			ImGui::Spacing();
			ImGui::Spacing();

			SceneRendererSettings& settings = SceneRenderer::GetSettings();

			ImGui::Text("RenderQueue");
			ImGui::SameLine();
			ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);

			int limit = stats.GeometryCount;
			ImGui::SliderInt("##RenderQueue", &limit, 0, stats.GeometryCount);
			settings.RenderQueueLimit = limit;
				
			ImGui::Text("DebugView");
			ImGui::SameLine();

			if (ImGui::BeginCombo("##DebugView", DebugViewToString(settings.DebugView).data()))
			{
				for (uint32 i = 0; i <= (uint32)DebugView::SHADOW_CASCADES; ++i)
				{
					DebugView view = (DebugView)i;

					bool isSelected = view == settings.DebugView;
					UI::Selectable(DebugViewToString(view), &isSelected, [this, view, &settings]()
						{
							settings.DebugView = view;
						});

				}

				ImGui::EndCombo();
			}

			ImGui::Spacing();
			ImGui::Spacing();

			UI::EndTreeNode();
		}

		if (UI::BeginTreeNode("Shadows", false))
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

			UI::EndTreeNode();
		}

		if (UI::BeginTreeNode("Bloom", false))
		{
			SceneRendererSettings& settings = SceneRenderer::GetSettings();

			ImGui::Text("Enable Bloom"); ImGui::SameLine(); ImGui::Checkbox("##Enable Shadows", &settings.BloomSettings.EnableBloom);
			ImGui::Text("Intensity"); ImGui::SameLine(); ImGui::DragFloat("##Intensity", &settings.BloomSettings.Intensity, 0.1f);
			ImGui::Text("Threshold"); ImGui::SameLine(); ImGui::DragFloat("##Threshold", &settings.BloomSettings.Threshold, 0.1f);
			ImGui::Text("Knee"); ImGui::SameLine(); ImGui::DragFloat("##Knee", &settings.BloomSettings.Knee, 0.1f);
			ImGui::Text("DirtIntensity"); ImGui::SameLine(); ImGui::DragFloat("##DirtIntensity", &settings.BloomSettings.DirtIntensity, 0.1f);

			ImGui::Text("Dirt Texture"); 
			ImGui::SameLine();
			auto dirtTexture = settings.BloomSettings.DirtTexture ? settings.BloomSettings.DirtTexture : Renderer::GetWhiteTexture();
			ImGui::Image(dirtTexture->GetRendererID(), { 50, 50 });

			UI::EndTreeNode();
		}

		if (UI::BeginTreeNode("Other", false))
		{
			SceneRendererSettings& settings = SceneRenderer::GetSettings();

			ImGui::Text("Antialiasing");
			ImGui::SameLine();
			if (ImGui::BeginCombo("##Antialiasing", AntialisingToString(settings.AntialisingMethod).data()))
			{
				for (uint32 i = 0; i <= (uint32)Antialising::MSAA_8X; ++i)
				{
					Antialising antialising = (Antialising)i;

					bool isSelected = antialising == settings.AntialisingMethod;
					UI::Selectable(AntialisingToString(antialising), &isSelected, [this, antialising, &settings]()
						{
							settings.AntialisingMethod = antialising;
						});

				}

				ImGui::EndCombo();
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

			UI::EndTreeNode();
		}

		ImGui::End();
	}
}
