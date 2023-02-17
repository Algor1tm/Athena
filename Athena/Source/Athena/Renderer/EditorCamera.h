#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Time.h"

#include "Athena/Input/Events/Event.h"
#include "Athena/Input/Events/MouseEvent.h"

#include "Athena/Math/Matrix.h"
#include "Athena/Math/Vector.h"
#include "Athena/Math/Quaternion.h"
#include "Athena/Math/Projections.h"

#include "Athena/Renderer/Camera.h"


namespace Athena
{
	class ATHENA_API EditorCamera : public Camera
	{
	public:
		virtual ~EditorCamera() = default;

		const Matrix4& GetViewMatrix() const { return m_ViewMatrix; }

		virtual void OnUpdate(Time frameTime) = 0;
		virtual void OnEvent(Event& event) = 0;

		virtual void SetViewportSize(uint32 width, uint32 height) = 0;
		virtual Vector3 GetPosition() const = 0;

		void SetMoveSpeedLevel(float level) { m_MoveSpeedLevel = level; }

	protected:
		Matrix4 m_ViewMatrix = Matrix4::Identity();
		float m_MoveSpeedLevel = 0.5f;
	};


	class ATHENA_API OrthographicCamera : public EditorCamera
	{
	public:
		OrthographicCamera() = default;
		OrthographicCamera(float left, float right, float bottom, float top, bool rotation = false);
		OrthographicCamera(float aspectRatio, bool rotation = false);
		virtual ~OrthographicCamera() = default;

		void SetProjection(float left, float right, float bottom, float top);

		void EnableRotation(bool enable) { m_EnableRotation = enable; }

		virtual Vector3 GetPosition() const override { return m_Position; }
		inline float GetRotation() const { return m_Rotation; }

		inline void SetPosition(const Vector3& position) { m_Position = position; RecalculateView(); }
		inline void SetRotation(float rotation) { m_Rotation = rotation; RecalculateView(); }

		virtual void OnUpdate(Time frameTime) override;
		virtual void OnEvent(Event& event) override;

		virtual void SetViewportSize(uint32 width, uint32 height) override;

		inline void SetZoomLevel(float level) { m_ZoomLevel = level; RecalculateProjection(); }
		inline float GetZoomLevel() const { return m_ZoomLevel; }

		inline void SetCameraSpeed(float speed) { m_CameraSpeed = speed; }
		inline void SetCameraRotationSpeed(float speed) { m_CameraRotationSpeed = speed; }

	private:
		void RecalculateProjection();
		void RecalculateView();

		bool OnMouseScrolled(MouseScrolledEvent& event);

	private:
		Vector3 m_Position = Vector3(0);
		float m_Rotation = 0;
		bool m_EnableRotation = false;

		float m_AspectRatio = 1.778f;
		float m_ZoomLevel = 1.f;

		float m_CameraSpeed = 3.f;
		float m_CameraRotationSpeed = 1.f;
	};


	class ATHENA_API PerspectiveCameraBase
	{
	public:
		PerspectiveCameraBase(float fov, float aspectRatio, float nearClip, float farClip);

		void SetPitch(float pitch) { m_Pitch = pitch; }
		void SetYaw(float yaw) { m_Yaw = yaw; }

		float GetPitch() const { return m_Pitch; }
		float GetYaw() const { return m_Yaw; }

	protected:
		Matrix4 GetViewMatrix(const Vector3& position);
		Matrix4 GetProjectionMatrix();

		Vector2 GetViewportSize() const { return { m_ViewportWidth, m_ViewportHeight }; }
		void SetViewportSize(uint32 width, uint32 height);

		Vector2 UpdateMousePosition(Time frameTime);

		Vector3 GetUpDirection() const;
		Vector3 GetRightDirection() const;
		Vector3 GetForwardDirection() const;
		Quaternion GetOrientation() const;

	private:
		float m_FOV = 45.0f; float m_AspectRatio = 1.778f;
		float m_NearClip = 0.1f; float m_FarClip = 1000.0f;

		float m_Yaw = 0.f, m_Pitch = 0.0f;

		Vector2 m_InitialMousePosition = { 0.0f, 0.0f };
		float m_ViewportWidth = 1600, m_ViewportHeight = 900;
	};


	class ATHENA_API MeshViewerCamera : public EditorCamera, PerspectiveCameraBase
	{
	public:
		MeshViewerCamera() = default;
		MeshViewerCamera(float fov, float aspectRatio, float nearClip, float farClip);
		virtual ~MeshViewerCamera() = default;

		virtual void OnUpdate(Time frameTime) override;
		virtual void OnEvent(Event& event) override;

		inline float GetDistance() const { return m_Distance; }
		inline void SetDistance(float distance) { m_Distance = distance; }

		virtual void SetViewportSize(uint32 width, uint32 height) override;

		virtual Vector3 GetPosition() const override { return CalculatePosition(); }

		inline void Pan(const Vector3& offset) { m_FocalPoint += offset; }

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
		Vector3 m_FocalPoint = { 0.0f, 0.0f, 0.0f };
		float m_Distance = 0.0f;
	};


	class ATHENA_API FirstPersonCamera : public EditorCamera, PerspectiveCameraBase
	{
	public:
		FirstPersonCamera() = default;
		FirstPersonCamera(float fov, float aspectRatio, float nearClip, float farClip);

		virtual void OnUpdate(Time frameTime) override;
		virtual void OnEvent(Event& event) override;

		virtual void SetViewportSize(uint32 width, uint32 height) override;

		virtual Vector3 GetPosition() const override { return m_Position; }

	private:
		void RecalculateProjection();
		void RecalculateView();

		bool OnMouseScroll(MouseScrolledEvent& event);

		float MoveSpeed() const;
		float RotationSpeed() const;
		float ZoomSpeed() const;

	private:
		Vector3 m_Position = { 0.0f, 500.0f, 1000.0f };
	};
}
