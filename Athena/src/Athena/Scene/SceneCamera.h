#pragma once

#include "Athena/Renderer/Camera.h"


namespace Athena
{
	class ATHENA_API SceneCamera : public Camera
	{
	public:
		SceneCamera();

		void SetOrthographic(float size, float nearClip, float farClip);
		void SetViewportSize(uint32 width, uint32 height);

		float GetOrthographicSize() const { return m_OrthographicSize; }
		void SetOrthographicSize(float size) { m_OrthographicSize = size; RecalculateProjection(); }

	private:
		void RecalculateProjection();

	private:
		float m_OrthographicSize = 10.f;
		float m_OrthographicNear = -1.f, m_OrthographicFar = 1.f;

		float m_AspecRatio = 1.f;
	};
}
