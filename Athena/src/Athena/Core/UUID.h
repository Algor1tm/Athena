#pragma once

#include "Core.h"

#include <xhash>


namespace Athena
{
	class ATHENA_API UUID
	{
	public:
		UUID();
		UUID(uint64 uuid);

		operator uint64() const { return m_UUID; }

	private:
		uint64 m_UUID;
	};
}


namespace std
{
	template <>
	struct hash<Athena::UUID>
	{
		size_t operator()(const Athena::UUID& uuid) const
		{
			return hash<uint64_t>()((uint64_t)uuid);
		}
	};
}
