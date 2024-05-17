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
	bool OnKeyPressedEvent(KeyPressedEvent& event);

	void UpdateStats();
	void OnRender2D();
	void RenderUIOverlay();

private:
	Ref<SceneRenderer2D> m_Renderer2D;
 	Ref<SceneRenderer> m_SceneRenderer;
	Ref<Scene> m_Scene;
	FilePath m_ScenePath;

	float m_TimeToUpdate = 0.f;
	bool m_ShowDefaultStats = false;
	String m_DefaultStats;
	bool m_ShowDetailedStats = false;
	String m_DetailedStats;
};
