#include "atnpch.h"

#include "backends/imgui_impl_opengl3.h"

#include "ImGuiLayer.h"
#include "Athena/Application.h"

// TEMPORARY
#include "GLFW/glfw3.h"
#include "glad/glad.h"


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
		io.KeyMap[ImGuiKey_Space] = GLFW_KEY_SPACE;

		Application& app = Application::Get();
		io.DisplaySize = ImVec2((float)app.GetWindow().GetWidth(), (float)app.GetWindow().GetHeight());

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
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<MouseButtonPressedEvent>(BIND_MEMBER_EVENT_FN(ImGuiLayer::OnMouseButtonPressedEvent));
		dispatcher.Dispatch<MouseButtonReleasedEvent>(BIND_MEMBER_EVENT_FN(ImGuiLayer::OnMouseButtonReleasedEvent));
		dispatcher.Dispatch<MouseMovedEvent>(BIND_MEMBER_EVENT_FN(ImGuiLayer::OnMouseMovedEvent));
		dispatcher.Dispatch<MouseScrolledEvent>(BIND_MEMBER_EVENT_FN(ImGuiLayer::OnMouseScrolledEvent));
		dispatcher.Dispatch<KeyPressedEvent>(BIND_MEMBER_EVENT_FN(ImGuiLayer::OnKeyPressedEvent));
		dispatcher.Dispatch<KeyReleasedEvent>(BIND_MEMBER_EVENT_FN(ImGuiLayer::OnKeyReleasedEvent));
		dispatcher.Dispatch<KeyTypedEvent>(BIND_MEMBER_EVENT_FN(ImGuiLayer::OnKeyTypedEvent));
		dispatcher.Dispatch<WindowResizedEvent>(BIND_MEMBER_EVENT_FN(ImGuiLayer::OnWindowResizedEvent));
	}


	bool ImGuiLayer::OnMouseButtonPressedEvent(MouseButtonPressedEvent& event)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.MouseDown[event.GetMouseButton()] = true;

		return false;
	}


	bool ImGuiLayer::OnMouseButtonReleasedEvent(MouseButtonReleasedEvent& event)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.MouseDown[event.GetMouseButton()] = false;

		return false;
	}


	bool ImGuiLayer::OnMouseMovedEvent(MouseMovedEvent& event)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.MousePos = ImVec2(event.GetX(), event.GetY());

		return false;
	}


	bool ImGuiLayer::OnMouseScrolledEvent(MouseScrolledEvent& event)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.MouseWheelH += event.GetXOffset();
		io.MouseWheel += event.GetYOffset();

		return false;
	}


	bool ImGuiLayer::OnKeyPressedEvent(KeyPressedEvent& event)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.KeysDown[event.GetKeyCode()] = true;

		io.KeyCtrl = io.KeysDown[GLFW_KEY_LEFT_CONTROL] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
		io.KeyShift = io.KeysDown[GLFW_KEY_LEFT_SHIFT] || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
		io.KeyAlt = io.KeysDown[GLFW_KEY_LEFT_ALT] || io.KeysDown[GLFW_KEY_RIGHT_ALT];
		io.KeySuper = io.KeysDown[GLFW_KEY_LEFT_SUPER] || io.KeysDown[GLFW_KEY_RIGHT_SUPER];
		return false;
	}


	bool ImGuiLayer::OnKeyReleasedEvent(KeyReleasedEvent& event)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.KeysDown[event.GetKeyCode()] = false;

		return false;
	}


	bool ImGuiLayer::OnKeyTypedEvent(KeyTypedEvent& event)
	{
		ImGuiIO& io = ImGui::GetIO();
		int keycode = event.GetKeyCode();
		if(keycode >= 0)
			io.AddInputCharacter((unsigned int)keycode);

		return false;
	}


	bool ImGuiLayer::OnWindowResizedEvent(WindowResizedEvent& event)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = ImVec2((float)event.GetWidth(), (float)event.GetHeight());
		io.DisplayFramebufferScale = ImVec2(1.f, 1.f);
		glViewport(0, 0, event.GetWidth(), event.GetHeight());

		return false;
	}
}
