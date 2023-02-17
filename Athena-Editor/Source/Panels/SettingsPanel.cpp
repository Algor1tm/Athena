#include "SettingsPanel.h"

#include "Athena/Renderer/Renderer2D.h"

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
				UI::ShiftCursorX(2.f);
				ImGui::PushStyleColor(ImGuiCol_Button, UI::GetDarkColor());
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 10, 3 });
				if (ImGui::Button("Reload Shaders"))
				{
					Renderer::ReloadShaders();
				}
				ImGui::PopStyleVar();
				ImGui::PopStyleColor();

				ImGui::Text("Camera Speed");
				ImGui::SameLine();
				ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
				ImGui::SliderFloat("##CameraSpeed", &m_EditorSettings.m_CameraSpeedLevel, 0.f, 1.f);

				UI::EndTreeNode();
			}
		}

		ImGui::End();



		ImGui::Begin("Renderer");
		ImGui::PopStyleVar();

		if (UI::BeginTreeNode("Debug"))
		{
			auto stats = Renderer::GetStatistics();

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
			ImGui::SliderInt("##RenderQueue", &m_RenderQueueLimit, 0, stats.GeometryCount);
			Renderer::SetRenderQueueLimit(m_RenderQueueLimit);
				
			ImGui::Text("DebugView");
			ImGui::SameLine();

			if (ImGui::BeginCombo("##DebugView", DebugViewToString(Renderer::GetDebugView()).data()))
			{
				for (uint32 i = 0; i <= (uint32)DebugView::WIREFRAME; ++i)
				{
					DebugView view = (DebugView)i;

					bool isSelected = view == Renderer::GetDebugView();
					UI::Selectable(DebugViewToString(view), &isSelected, [this, view]()
						{
							Renderer::SetDebugView(view);
						});

				}

				ImGui::EndCombo();
			}

			ImGui::Spacing();
			ImGui::Spacing();

			ImGui::Text("Geometry Pass: %.3f", stats.GeometryPass.AsMilliseconds());
			ImGui::Text("Skybox Pass: %.3f", stats.SkyboxPass.AsMilliseconds());

			UI::EndTreeNode();
		}

		if (UI::BeginTreeNode("Other"))
		{
			if (ImGui::BeginCombo("##Antialiasing", AntialisingToString(Renderer::GetAntialiasingMethod()).data()))
			{
				for (uint32 i = 0; i <= (uint32)Antialising::MSAA_8X; ++i)
				{
					Antialising antialiasing = (Antialising)i;

					bool isSelected = antialiasing == Renderer::GetAntialiasingMethod();
					UI::Selectable(AntialisingToString(antialiasing), &isSelected, [this, antialiasing]()
						{
							Renderer::SetAntialiasingMethod(antialiasing);
						});

				}

				ImGui::EndCombo();
			}

			UI::EndTreeNode();
		}

		ImGui::End();
	}
}
