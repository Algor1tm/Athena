#include "atnpch.h"
#include "OrthographicCameraController.h"

#include "Athena/Input/Input.h"


namespace Athena
{
	OrthographicCameraController::OrthographicCameraController(float aspectRatio, bool rotation)
		: m_AspectRatio(aspectRatio), m_Rotation(rotation), 
		m_Camera(-m_ZoomLevel * m_AspectRatio, m_ZoomLevel * m_AspectRatio, -m_ZoomLevel, m_ZoomLevel)
	{

	}

	void OrthographicCameraController::OnUpdate(Time frameTime)
	{
		float seconds = frameTime.AsSeconds();

		if (Input::IsKeyPressed(Keyboard::A))
			m_CameraPosition.x -= m_CameraSpeed * seconds;
		else if (Input::IsKeyPressed(Keyboard::D))
			m_CameraPosition.x += m_CameraSpeed * seconds;
		else if (Input::IsKeyPressed(Keyboard::W))
			m_CameraPosition.y += m_CameraSpeed * seconds;
		else if (Input::IsKeyPressed(Keyboard::S))
			m_CameraPosition.y -= m_CameraSpeed * seconds;

		if (m_Rotation)
		{
			if (Input::IsKeyPressed(Keyboard::Q))
				m_CameraRotation += m_CameraRotationSpeed * seconds;
			else if (Input::IsKeyPressed(Keyboard::E))
				m_CameraRotation -= m_CameraRotationSpeed * seconds;

			m_Camera.SetRotation(m_CameraRotation);
		}

		m_Camera.SetPosition(m_CameraPosition);
	}

	void OrthographicCameraController::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<MouseScrolledEvent>(ATN_BIND_EVENT_FN(OrthographicCameraController::OnMouseScrolled));
		dispatcher.Dispatch<WindowResizeEvent>(ATN_BIND_EVENT_FN(OrthographicCameraController::OnWindowResized));
	}
	
	void OrthographicCameraController::Resize(uint32 width, uint32 height)
	{
		m_AspectRatio = (float)width / (float)height;
		RecalculateView();
	}

	void OrthographicCameraController::RecalculateView()
	{
		m_Camera.SetProjection(-m_ZoomLevel * m_AspectRatio, m_ZoomLevel * m_AspectRatio, -m_ZoomLevel, m_ZoomLevel);
	}

	bool OrthographicCameraController::OnMouseScrolled(MouseScrolledEvent& event)
	{
		m_ZoomLevel -= event.GetYOffset() * 0.4f;
		m_ZoomLevel = Math::Max(m_ZoomLevel, 0.2f);
		m_CameraSpeed = m_ZoomLevel * 1.5f;

		RecalculateView();

		return false;
	}

	bool OrthographicCameraController::OnWindowResized(WindowResizeEvent& event)
	{
		Resize(event.GetWidth(), event.GetHeight());

		return false;
	}
}
