#pragma once

#include "Athena.h"


class SandBox2D: public Athena::Layer
{
public:
	SandBox2D();

	void OnAttach() override;
	void OnDetach() override;

	void OnUpdate(Athena::Time frameTime) override;
	void OnImGuiRender() override;
	void OnEvent(Athena::Event& event) override;

private:
	Athena::Color m_SquareColor;
	Athena::Ref<Athena::Texture2D> m_CheckerBoard;
	Athena::Ref<Athena::Texture2D> m_KomodoHype;

	Athena::OrthographicCameraController m_CameraController;
};
