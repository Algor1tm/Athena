#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Time.h"

#include "Athena/Input/Events/Event.h"
#include "Athena/Input/Events/ApplicationEvent.h"
#include "Athena/Input/Events/MouseEvent.h"

#include "Athena/Renderer/OrthographicCamera.h"


namespace Athena
{
	class ATHENA_API OrthographicCameraController
	{
	public:
		OrthographicCameraController(float aspectRatio, bool rotation = true);

		void OnUpdate(Time frameTime);
		void OnEvent(Event& event);

		void Resize(uint32 width, uint32 height);

		inline OrthographicCamera& GetCamera() { return m_Camera; };
		inline const OrthographicCamera& GetCamera() const { return m_Camera; };

		inline void SetZoomLevel(float level) { m_ZoomLevel = level; RecalculateView(); }
		inline float GetZoomLevel() const { return m_ZoomLevel; }

	private:
		void RecalculateView();

		bool OnMouseScrolled(MouseScrolledEvent& event);
		bool OnWindowResized(WindowResizeEvent& event);

	private:
		float m_AspectRatio;
		float m_ZoomLevel = 1.f;
		float m_Rotation;

		OrthographicCamera m_Camera;
		Vector3 m_CameraPosition = { 0.f, 0.f, 0.f };
		float m_CameraRotation = 0.f;

		float m_CameraSpeed = 3.f;
		float m_CameraRotationSpeed = 1.f;
	};
}
