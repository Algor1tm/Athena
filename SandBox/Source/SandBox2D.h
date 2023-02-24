#pragma once

#include <Athena.h>

using namespace Athena;

class SandBox2D : public Layer
{
public:
	SandBox2D();

	void OnAttach() override;
	void OnDetach() override;

	void OnUpdate(Time frameTime) override;
	void OnEvent(Event& event) override;

private:
	bool OnWindowResize(WindowResizeEvent& event);

private:
	Ref<Scene> m_Scene;
};
