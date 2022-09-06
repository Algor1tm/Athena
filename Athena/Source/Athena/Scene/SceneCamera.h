#pragma once

#include "Athena/Renderer/Camera.h"

#include "Athena/Math/Trigonometric.h"


namespace Athena
{
	class ATHENA_API SceneCamera : public Camera
	{
	public:
		enum class ProjectionType { Perspective = 0, Orthographic = 1 };

		struct OrthographicDESC
		{
			float Size = 10.f;
			float NearClip = -1.f;
			float FarClip = 1.f;
		};

		struct PerspectiveDESC
		{
			float VerticalFOV = Math::Radians(45.f);
			float NearClip = 0.01f;
			float FarClip = 1000.f;
		};

	public:
		SceneCamera(ProjectionType type = ProjectionType::Orthographic);

		void SetViewportSize(uint32 width, uint32 height);

		float GetOrthographicSize() const { return m_OrthoData.Size; }
		void SetOrthographicSize(float size) { m_OrthoData.Size = size; RecalculateProjection(); }

		const OrthographicDESC& GetOrthographicData() const { return m_OrthoData; }
		const PerspectiveDESC& GetPerspectiveData() const { return m_PerspectiveData; }

		void SetOrthographicData(const OrthographicDESC& desc) { m_OrthoData = desc; RecalculateProjection(); }
		void SetPerspectiveData(const PerspectiveDESC& desc) { m_PerspectiveData = desc; RecalculateProjection(); }

		ProjectionType GetProjectionType() const { return m_ProjectionType; }
		void SetProjectionType(ProjectionType type) { m_ProjectionType = type; RecalculateProjection(); }

	private:
		void RecalculateProjection();

	private:
		ProjectionType m_ProjectionType = ProjectionType::Orthographic;

		OrthographicDESC m_OrthoData;
		PerspectiveDESC m_PerspectiveData;

		float m_AspecRatio = 1.f;
	};
}
