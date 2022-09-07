#include "atnpch.h"
#include "UUID.h"

#include "Athena/Math/Random.h"


namespace Athena
{
	UUID::UUID()
		: m_UUID(Math::Random::UInt64())
	{

	}

	UUID::UUID(uint64 uuid)
		: m_UUID(uuid)
	{

	}
}
