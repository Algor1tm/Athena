#include "atnpch.h"
#include "EditorCamera.h"

#include "Athena/Math/Projections.h"
#include "Athena/Input/Input.h"


namespace Athena
{
	EditorCamera::EditorCamera(float fov, float aspectRatio, float nearClip, float farClip)
		: Camera(Math::Perspective(fov, aspectRatio, nearClip, farClip)), 
	    m_FOV(fov), m_AspectRatio(aspectRatio), m_NearClip(nearClip), m_FarClip(farClip)
	{
		RecalculateView();
	}

	void EditorCamera::RecalculateProjection()
	{
		m_AspectRatio = m_ViewportWidth / m_ViewportHeight;
		m_Projection = Math::Perspective(m_FOV, m_AspectRatio, m_NearClip, m_FarClip);
	}

	void EditorCamera::RecalculateView()
	{
		m_Position = CalculatePosition();

		m_ViewMatrix = Math::ToMat4(GetOrientation());
		m_ViewMatrix = Math::AffineInverse(m_ViewMatrix.Translate(m_Position));
	}

	Vector2 EditorCamera::PanSpeed() const
	{
		float x = Math::Min(m_ViewportWidth / 1000.f, 2.4f);
		float xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;

		float y = Math::Min(m_ViewportHeight / 1000.0f, 2.4f);
		float yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;

		return { xFactor, yFactor };
	}

	float EditorCamera::RotationSpeed() const
	{
		return 1.8f;
	}

	float EditorCamera::ZoomSpeed() const
	{
		float distance = m_Distance * 0.4f;
		distance = Math::Max(distance, 0.0f);
		float speed = distance * distance;
		speed = Math::Min(speed, 100.0f);
		return speed;
	}

	void EditorCamera::OnUpdate(Time frameTime)
	{
		Vector2 mousePos = Input::GetMousePosition();
		Vector2 delta = (mousePos - m_InitialMousePosition) * 0.003f;
		m_InitialMousePosition = mousePos;

		bool changed = false;

		if (Input::IsMouseButtonPressed(Mouse::Wheel))
		{
			MousePan(delta);
			changed = true;
		}
		else if (Input::IsMouseButtonPressed(Mouse::Right))
		{
			MouseRotate(delta);
			changed = true;
		}

		if(changed)
			RecalculateView();
	}

	void EditorCamera::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<MouseScrolledEvent>(ATN_BIND_EVENT_FN(EditorCamera::OnMouseScroll));
	}

	bool EditorCamera::OnMouseScroll(MouseScrolledEvent& event)
	{
		float delta = event.GetYOffset() * 0.1f;
		MouseZoom(delta);
		RecalculateView();
		return false;
	}

	void EditorCamera::MousePan(const Vector2& delta)
	{
		auto [xSpeed, ySpeed] = PanSpeed();
		m_FocalPoint += -GetRightDirection() * delta.x * xSpeed * m_Distance;
		m_FocalPoint += GetUpDirection() * delta.y * ySpeed * m_Distance;
	}

	void EditorCamera::MouseRotate(const Vector2& delta)
	{
		float yawSign = Math::Sign(GetUpDirection().y);
		m_Yaw += yawSign * delta.x * RotationSpeed();
		m_Pitch += delta.y * RotationSpeed();
	}

	void EditorCamera::MouseZoom(float delta)
	{
		m_Distance -= delta * ZoomSpeed();
		if (m_Distance < 1.0f)
		{
			m_FocalPoint += GetForwardDirection();
			m_Distance = 1.0f;
		}
	}

	Vector3 EditorCamera::GetUpDirection() const
	{
		return GetOrientation() * Vector3::up();
	}

	Vector3 EditorCamera::GetRightDirection() const
	{
		return GetOrientation() * Vector3::right();
	}

	Vector3 EditorCamera::GetForwardDirection() const
	{
		return GetOrientation() * Vector3::forward();
	}

	Vector3 EditorCamera::CalculatePosition() const
	{
		return m_FocalPoint - GetForwardDirection() * m_Distance;
	}

	Quat EditorCamera::GetOrientation() const
	{
		return Math::ToQuat(Vector3(-m_Pitch, -m_Yaw, 0.0f));
	}
}
