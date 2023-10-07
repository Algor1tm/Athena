#pragma once

#include <Athena.h>

using namespace Athena;

class SandBoxLayer : public Layer
{
public:
	SandBoxLayer(const FilePath& scenePath);

	void OnAttach() override;
	void OnDetach() override;

	void OnUpdate(Time frameTime) override;
	void OnEvent(Event& event) override;

private:
	bool OnWindowResize(WindowResizeEvent& event);

private:
	Ref<SceneRenderer> m_SceneRenderer;
	Ref<Scene> m_Scene;
	FilePath m_ScenePath;
};
