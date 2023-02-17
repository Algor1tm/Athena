#include "EditorCamera.h"

#include "Athena/Input/Input.h"

#include "Athena/Math/Transforms.h"


namespace Athena
{
	OrthographicCamera::OrthographicCamera(float left, float right, float bottom, float top, bool rotation)
		: m_Position(0.f), m_Rotation(0.f), m_EnableRotation(rotation)
	{
		RecalculateView();
		RecalculateProjection();
	}

	OrthographicCamera::OrthographicCamera(float aspectRatio, bool rotation)
		: m_AspectRatio(aspectRatio), m_EnableRotation(rotation)
	{
		RecalculateView();
		RecalculateProjection();
	}

	void OrthographicCamera::SetProjection(float left, float right, float bottom, float top)
	{
		m_ProjectionMatrix = Math::Ortho(left, right, bottom, top, -1.f, 1.f);
	}

	void OrthographicCamera::RecalculateView()
	{
		Matrix4 transform = Math::RotateMatrix(m_Rotation, Vector3(0, 0, 1)) * Math::TranslateMatrix(m_Position);
		m_ViewMatrix = Math::AffineInverse(transform);
	}

	void OrthographicCamera::RecalculateProjection()
	{
		SetProjection(-m_ZoomLevel * m_AspectRatio, m_ZoomLevel * m_AspectRatio, -m_ZoomLevel, m_ZoomLevel);
	}

	void OrthographicCamera::OnUpdate(Time frameTime)
	{
		float seconds = frameTime.AsSeconds();

		if (Input::IsKeyPressed(Keyboard::A))
			m_Position.x -= m_CameraSpeed * seconds;
		else if (Input::IsKeyPressed(Keyboard::D))
			m_Position.x += m_CameraSpeed * seconds;
		else if (Input::IsKeyPressed(Keyboard::W))
			m_Position.y += m_CameraSpeed * seconds;
		else if (Input::IsKeyPressed(Keyboard::S))
			m_Position.y -= m_CameraSpeed * seconds;

		if (m_EnableRotation)
		{
			if (Input::IsKeyPressed(Keyboard::Q))
				m_Rotation += m_CameraRotationSpeed * seconds;
			else if (Input::IsKeyPressed(Keyboard::E))
				m_Rotation -= m_CameraRotationSpeed * seconds;
		}

		RecalculateView();
	}

	void OrthographicCamera::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<MouseScrolledEvent>(ATN_BIND_EVENT_FN(OrthographicCamera::OnMouseScrolled));
	}

	void OrthographicCamera::SetViewportSize(uint32 width, uint32 height)
	{
		m_AspectRatio = (float)width / (float)height;
		SetProjection(-m_ZoomLevel * m_AspectRatio, m_ZoomLevel * m_AspectRatio, -m_ZoomLevel, m_ZoomLevel);
	}

	bool OrthographicCamera::OnMouseScrolled(MouseScrolledEvent& event)
	{
		m_ZoomLevel -= event.GetYOffset() * 0.4f;
		m_ZoomLevel = Math::Max(m_ZoomLevel, 0.2f);
		m_CameraSpeed = m_ZoomLevel * 1.5f;

		RecalculateProjection();

		return false;
	}


	PerspectiveCameraBase::PerspectiveCameraBase(float fov, float aspectRatio, float nearClip, float farClip)
		: m_FOV(fov), m_AspectRatio(aspectRatio), m_NearClip(nearClip), m_FarClip(farClip)
	{

	}


	Matrix4 PerspectiveCameraBase::GetViewMatrix(const Vector3& position)
	{
		Matrix4 view = Math::ToMat4(GetOrientation());
		view = Math::AffineInverse(view.Translate(position));

		return view;
	}

	Matrix4 PerspectiveCameraBase::GetProjectionMatrix()
	{
		m_AspectRatio = m_ViewportWidth / m_ViewportHeight;
		return Math::Perspective(m_FOV, m_AspectRatio, m_NearClip, m_FarClip);
	}

	void PerspectiveCameraBase::SetViewportSize(uint32 width, uint32 height)
	{
		m_ViewportWidth = (float)width;
		m_ViewportHeight = (float)height;
	}

	Vector2 PerspectiveCameraBase::UpdateMousePosition(Time frameTime)
	{
		Vector2 mousePos = Input::GetMousePosition();
		Vector2 delta = (mousePos - m_InitialMousePosition) * frameTime.AsSeconds();
		m_InitialMousePosition = mousePos;

		return delta;
	}

	Vector3 PerspectiveCameraBase::GetUpDirection() const
	{
		return GetOrientation() * Vector3::Up();
	}

	Vector3 PerspectiveCameraBase::GetRightDirection() const
	{
		return GetOrientation() * Vector3::Right();
	}

	Vector3 PerspectiveCameraBase::GetForwardDirection() const
	{
		return GetOrientation() * Vector3::Forward();
	}

	Quaternion PerspectiveCameraBase::GetOrientation() const
	{
		return Math::ToQuat(Vector3(-m_Pitch, -m_Yaw, 0.0f));
	}



	MeshViewerCamera::MeshViewerCamera(float fov, float aspectRatio, float nearClip, float farClip)
		: PerspectiveCameraBase(fov, aspectRatio, nearClip, farClip)
	{
		RecalculateProjection();
		RecalculateView();
	}

	void MeshViewerCamera::RecalculateProjection()
	{
		m_ProjectionMatrix = PerspectiveCameraBase::GetProjectionMatrix();
	}

	void MeshViewerCamera::RecalculateView()
	{
		Vector3 position = CalculatePosition();
		m_ViewMatrix = PerspectiveCameraBase::GetViewMatrix(position);
	}

	Vector2 MeshViewerCamera::PanSpeed() const
	{
		Vector2 viewport = GetViewportSize();
		float x = Math::Min(viewport.x / 1000.f, 2.4f);
		float xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;

		float y = Math::Min(viewport.y / 1000.0f, 2.4f);
		float yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;

		return { xFactor, yFactor };
	}

	float MeshViewerCamera::RotationSpeed() const
	{
		return 0.3f;
	}

	float MeshViewerCamera::ZoomSpeed() const
	{
		float distance = m_Distance * 2.4f;
		distance = Math::Max(distance, 0.0f);
		float speed = Math::Pow(distance, 1.5f);
		speed = Math::Min(speed, 1000.0f);
		return speed * m_MoveSpeedLevel;
	}

	void MeshViewerCamera::OnUpdate(Time frameTime)
	{
		Vector2 delta = UpdateMousePosition(frameTime);

		if (Input::IsMouseButtonPressed(Mouse::Middle))
		{
			MousePan(delta * PanSpeed());
		}
		else if (Input::IsMouseButtonPressed(Mouse::Right))
		{
			MouseRotate(delta * RotationSpeed());
		}

		RecalculateView();
	}

	void MeshViewerCamera::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<MouseScrolledEvent>(ATN_BIND_EVENT_FN(MeshViewerCamera::OnMouseScroll));
	}

	bool MeshViewerCamera::OnMouseScroll(MouseScrolledEvent& event)
	{
		float delta = event.GetYOffset() * 0.1f;
		MouseZoom(delta);
		return false;
	}

	void MeshViewerCamera::MousePan(const Vector2& delta)
	{
		m_FocalPoint += -GetRightDirection() * delta.x * m_Distance;
		m_FocalPoint += GetUpDirection() * delta.y * m_Distance;
	}

	void MeshViewerCamera::MouseRotate(const Vector2& delta)
	{
		float yawSign = Math::Sign(GetUpDirection().y);
		float yaw = yawSign * delta.x;
		float pitch = delta.y;

		SetYaw(GetYaw() + yaw);
		SetPitch(GetPitch() + pitch);
	}

	void MeshViewerCamera::MouseZoom(float delta)
	{
		m_Distance -= delta * ZoomSpeed();
		if (m_Distance < 1.0f)
		{
			m_FocalPoint += GetForwardDirection();
			m_Distance = 1.0f;
		}
	}

	void MeshViewerCamera::SetViewportSize(uint32 width, uint32 height)
	{
		PerspectiveCameraBase::SetViewportSize(width, height);
		RecalculateProjection();
	}

	Vector3 MeshViewerCamera::CalculatePosition() const
	{
		return m_FocalPoint - GetForwardDirection() * m_Distance;
	}



	FirstPersonCamera::FirstPersonCamera(float fov, float aspectRatio, float nearClip, float farClip)
		: PerspectiveCameraBase(fov, aspectRatio, nearClip, farClip)
	{
		RecalculateProjection();
		RecalculateView();
	}

	void FirstPersonCamera::OnUpdate(Time frameTime)
	{
		Vector2 delta = UpdateMousePosition(frameTime) * RotationSpeed();
		delta = Math::Clamp(delta, -0.1f, 0.1f);

		if (Input::IsMouseButtonPressed(Mouse::Right))
		{
			float yawSign = Math::Sign(GetUpDirection().y);
			float yaw = yawSign * delta.x;
			float pitch = delta.y;

			SetYaw(GetYaw() + yaw);
			SetPitch(GetPitch() + pitch);

			Vector3 direction = { 0.f, 0.f, 0.f };

			if (Input::IsKeyPressed(Keyboard::W))
				direction += GetForwardDirection();
			else if (Input::IsKeyPressed(Keyboard::S))
				direction += -GetForwardDirection();

			else if (Input::IsKeyPressed(Keyboard::A))
				direction += -GetRightDirection();
			else if (Input::IsKeyPressed(Keyboard::D))
				direction += GetRightDirection();

			else if (Input::IsKeyPressed(Keyboard::E))
				direction += Vector3::Up() / 1.5f;
			else if (Input::IsKeyPressed(Keyboard::Q))
				direction += Vector3::Down() / 1.5f;

			m_Position += direction * frameTime.AsSeconds() * MoveSpeed();
		}

		RecalculateView();
	}

	void FirstPersonCamera::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<MouseScrolledEvent>(ATN_BIND_EVENT_FN(FirstPersonCamera::OnMouseScroll));
	}

	void FirstPersonCamera::SetViewportSize(uint32 width, uint32 height)
	{
		PerspectiveCameraBase::SetViewportSize(width, height);
		RecalculateProjection();
	}

	void FirstPersonCamera::RecalculateProjection()
	{
		m_ProjectionMatrix = PerspectiveCameraBase::GetProjectionMatrix();
	}

	void FirstPersonCamera::RecalculateView()
	{
		m_ViewMatrix = PerspectiveCameraBase::GetViewMatrix(m_Position);
	}

	bool FirstPersonCamera::OnMouseScroll(MouseScrolledEvent& event)
	{
		Vector3 direction = GetForwardDirection();
		m_Position += direction * event.GetYOffset() * ZoomSpeed();

		return false;
	}

	float FirstPersonCamera::MoveSpeed() const
	{
		return 7000.f * m_MoveSpeedLevel;
	}

	float FirstPersonCamera::RotationSpeed() const
	{
		return 0.15f;
	}

	float FirstPersonCamera::ZoomSpeed() const
	{
		return 200.f;
	}
}
