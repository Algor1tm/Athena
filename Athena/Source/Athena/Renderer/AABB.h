#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Math/Vector.h"
#include "Athena/Math/Matrix.h"


namespace Athena
{
	class ATHENA_API AABB
	{
	public:
		AABB();
		AABB(const Vector3& min, const Vector3& max);

		AABB Transform(const Matrix4& transform) const;

		void Extend(const Vector3& point);
		void Extend(const AABB& other);

		bool Contains(const Vector3& point);
		bool Contains(const AABB& other);

		const Vector3& GetMinPoint() const { return m_MinPoint; }
		const Vector3& GetMaxPoint() const { return m_MaxPoint; }

	private:
		Vector3 m_MaxPoint;
		Vector3 m_MinPoint;
	};
}