#include "SettingsPanel.h"

#include "Athena/Renderer/SceneRenderer.h"

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
		case DebugView::NORMALS: return "Normals";
		case DebugView::WIREFRAME: return "Wireframe";
		case DebugView::SHOW_CASCADES: return "ShowShadowCascades";
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
				//UI::ShiftCursorX(2.f);


				ImGui::Text("Camera Speed");
				ImGui::SameLine();
				ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
				ImGui::SliderFloat("##CameraSpeed", &m_EditorSettings.m_CameraSpeedLevel, 0.f, 10.f);

				UI::EndTreeNode();
			}
		}

		ImGui::End();



		ImGui::Begin("Renderer");
		ImGui::PopStyleVar();

		if (UI::BeginTreeNode("Debug"))
		{
			auto stats = SceneRenderer::GetStatistics();

			ImGui::Text("Draw Calls: %d", stats.DrawCalls);
			ImGui::Text("Directional Lights: %d", stats.DirectionalLightsCount);
			ImGui::Text("Point Lights: %d", stats.PointLightsCount);

			ImGui::Spacing();
			ImGui::Spacing();

			if (m_RenderQueueLimit < 0 || m_RenderGeometryCount != stats.GeometryCount)
				m_RenderQueueLimit = stats.GeometryCount;

			m_RenderGeometryCount = stats.GeometryCount;

			ImGui::Text("RenderQueue");
			ImGui::SameLine();
			ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
			ImGui::SliderInt("##RenderQueue", &m_RenderQueueLimit, 0, stats.GeometryCount);
			SceneRenderer::SetRenderQueueLimit(m_RenderQueueLimit);
				
			ImGui::Text("DebugView");
			ImGui::SameLine();

			if (ImGui::BeginCombo("##DebugView", DebugViewToString(SceneRenderer::GetDebugView()).data()))
			{
				for (uint32 i = 0; i <= (uint32)DebugView::SHOW_CASCADES; ++i)
				{
					DebugView view = (DebugView)i;

					bool isSelected = view == SceneRenderer::GetDebugView();
					UI::Selectable(DebugViewToString(view), &isSelected, [this, view]()
						{
							SceneRenderer::SetDebugView(view);
						});

				}

				ImGui::EndCombo();
			}

			ImGui::Spacing();
			ImGui::Spacing();

			UI::EndTreeNode();
		}

		if (UI::BeginTreeNode("Shadows"))
		{
			auto settings = SceneRenderer::GetShadowSettings();
			ImGui::Text("Enable Shadows"); ImGui::SameLine(); ImGui::Checkbox("##Enable Shadows", &settings.EnableShadows);
			ImGui::Text("Soft Shadows"); ImGui::SameLine(); ImGui::Checkbox("##Soft Shadows", &settings.SoftShadows);
			ImGui::Text("Light Size"); ImGui::SameLine(); ImGui::DragFloat("##Light Size", &settings.LightSize, 0.025f);
			ImGui::Text("Max Distance"); ImGui::SameLine(); ImGui::DragFloat("##Max Distance", &settings.MaxDistance);
			ImGui::Text("Fade Out"); ImGui::SameLine(); ImGui::DragFloat("##Fade Out", &settings.FadeOut);
			ImGui::Text("Split Factor"); ImGui::SameLine(); ImGui::SliderFloat("##Split Factor", &settings.ExponentialSplitFactor, 0.f, 1.f);
			ImGui::Text("NearPlaneOffset"); ImGui::SameLine(); ImGui::DragFloat("##NearPlaneOffset", &settings.NearPlaneOffset);
			ImGui::Text("FarPlaneOffset"); ImGui::SameLine(); ImGui::DragFloat("##FarPlaneOffset", &settings.FarPlaneOffset);
			SceneRenderer::SetShadowSettings(settings);

			UI::EndTreeNode();
		}

		if (UI::BeginTreeNode("Other"))
		{
			ImGui::Text("Antialiasing");
			ImGui::SameLine();
			if (ImGui::BeginCombo("##Antialiasing", AntialisingToString(SceneRenderer::GetAntialiasingMethod()).data()))
			{
				for (uint32 i = 0; i <= (uint32)Antialising::MSAA_8X; ++i)
				{
					Antialising antialiasing = (Antialising)i;

					bool isSelected = antialiasing == SceneRenderer::GetAntialiasingMethod();
					UI::Selectable(AntialisingToString(antialiasing), &isSelected, [this, antialiasing]()
						{
							SceneRenderer::SetAntialiasingMethod(antialiasing);
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
				SceneRenderer::ReloadShaders();
			}
			ImGui::PopStyleVar();
			ImGui::PopStyleColor();

			UI::EndTreeNode();
		}

		ImGui::End();
	}
}
