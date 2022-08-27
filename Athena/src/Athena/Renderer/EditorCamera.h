#pragma once

#include "Athena/Core/Time.h"
#include "Athena/Renderer/Camera.h"
#include "Athena/Events/Event.h"
#include "Athena/Events/MouseEvent.h"

#include "Athena/Math/Matrix.h"
#include "Athena/Math/Vector.h"
#include "Athena/Math/Quaternion.h"


namespace Athena
{
	class ATHENA_API EditorCamera : public Camera
	{
	public:
		EditorCamera() = default;
		EditorCamera(float fov, float aspectRatio, float nearClip, float farClip);

		void OnUpdate(Time frameTime);
		void OnEvent(Event& e);

		inline float GetDistance() const { return m_Distance; }
		inline void SetDistance(float distance) { m_Distance = distance; RecalculateView(); }

		inline void SetViewportSize(uint32 width, uint32 height) 
		{ m_ViewportWidth = (float)width; m_ViewportHeight = (float)height; RecalculateProjection(); }

		inline void SetPitch(float pitch) { m_Pitch = pitch; RecalculateView(); }
		inline void SetYaw(float yaw) { m_Yaw = yaw; RecalculateView(); }

		const Matrix4& GetViewMatrix() const { return m_ViewMatrix; }
		Matrix4 GetViewProjection() const { return m_ViewMatrix * m_Projection; }
			
		Vector3 GetUpDirection() const;
		Vector3 GetRightDirection() const;
		Vector3 GetForwardDirection() const;
		const Vector3& GetPosition() const { return m_Position; }
		Quat GetOrientation() const;

		float GetPitch() const { return m_Pitch; }
		float GetYaw() const { return m_Yaw; }

	private:
		void RecalculateProjection();
		void RecalculateView();

		bool OnMouseScroll(MouseScrolledEvent& event);

		void MousePan(const Vector2& delta);
		void MouseRotate(const Vector2& delta);
		void MouseZoom(float delta);

		Vector3 CalculatePosition() const;

		Vector2 PanSpeed() const;
		float RotationSpeed() const;
		float ZoomSpeed() const;

	private:
		float m_FOV = 45.0f, m_AspectRatio = 1.778f, m_NearClip = 0.1f, m_FarClip = 1000.0f;

		Matrix4 m_ViewMatrix;
		Vector3 m_Position = { 0.0f, 0.0f, 0.0f };
		Vector3 m_FocalPoint = { 0.0f, 0.0f, 0.0f };

		Vector2 m_InitialMousePosition = { 0.0f, 0.0f };

		float m_Distance = 10.0f;
		float m_Pitch = 0.0f, m_Yaw = 0.0f;

		float m_ViewportWidth = 1280, m_ViewportHeight = 720;
	};
}
