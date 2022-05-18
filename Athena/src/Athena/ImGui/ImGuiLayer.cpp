#include "atnpch.h"

#include "Impl/imgui_impl_opengl3.h"
#include "GLFW/glfw3.h"

#include "ImGuiLayer.h"
#include "Athena/Application.h"


namespace Athena
{
	ImGuiLayer::ImGuiLayer()
		: Layer("ImGuiLayer")
	{

	}


	ImGuiLayer::~ImGuiLayer()
	{

	}


	void ImGuiLayer::OnAttach()
	{
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; 
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

		ImGui::StyleColorsDark();

		ImGui_ImplOpenGL3_Init("#version 410");
	}


	void ImGuiLayer::OnDetach()
	{

	}


	void ImGuiLayer::OnUpdate()
	{
		ImGuiIO& io = ImGui::GetIO();
		Application& app = Application::Get();
		io.DisplaySize = ImVec2(app.GetWindow().GetWidth(), app.GetWindow().GetHeight());

		float time = (float)glfwGetTime();
		io.DeltaTime = m_Time > 0.f ? (time - m_Time) : (1.f / 60.f);
		m_Time = time;

		ImGui_ImplOpenGL3_NewFrame();
		ImGui::NewFrame();

		static bool show = true;
		ImGui::ShowDemoWindow(&show);

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}


	void ImGuiLayer::OnEvent(Event& event)
	{

	}

}
