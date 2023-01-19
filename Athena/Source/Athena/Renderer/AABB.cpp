#include "AABB.h"

#include "Athena/Math/Limits.h"


namespace Athena
{
	AABB::AABB()
		: m_MinPoint(Vector3(1.f)* Math::MaxValue<float>()), m_MaxPoint(Vector3(-1.f)* Math::MaxValue<float>())
	{

	}

	AABB::AABB(const Vector3& min, const Vector3& max)
		: AABB()
	{
		Extend(min);
		Extend(max);
	}

	AABB AABB::Transform(const Matrix4& transform) const
	{
		std::array<Vector3, 8> corners;

		corners[0] = m_MinPoint;
		corners[1] = Vector3(m_MinPoint.x, m_MinPoint.y, m_MaxPoint.z);
		corners[2] = Vector3(m_MinPoint.x, m_MaxPoint.y, m_MinPoint.z);
		corners[3] = Vector3(m_MaxPoint.x, m_MinPoint.y, m_MinPoint.z);
		corners[4] = Vector3(m_MinPoint.x, m_MaxPoint.y, m_MaxPoint.z);
		corners[5] = Vector3(m_MaxPoint.x, m_MinPoint.y, m_MaxPoint.z);
		corners[6] = Vector3(m_MaxPoint.x, m_MaxPoint.y, m_MinPoint.z);
		corners[7] = m_MaxPoint;

		AABB result;
		for (SIZE_T i = 0; i < corners.size(); ++i)
		{
			Vector3 transformed = corners[i] * transform;
			result.Extend(transformed);
		}

		return result;
	}

	void AABB::Extend(const Vector3& point)
	{
		m_MinPoint = Math::Min(point, m_MinPoint);
		m_MaxPoint = Math::Max(point, m_MaxPoint);
	}

	void AABB::Extend(const AABB& other)
	{
		Extend(other.m_MinPoint);
		Extend(other.m_MaxPoint);
	}

	bool AABB::Contains(const Vector3& point)
	{
		return (m_MinPoint == Math::Min(point, m_MinPoint)) &&
			(m_MaxPoint == Math::Max(point, m_MaxPoint));
	}

	bool AABB::Contains(const AABB& other)
	{
		return Contains(other.m_MinPoint) && Contains(other.m_MaxPoint);
	}
}
