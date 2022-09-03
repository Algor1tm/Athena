#pragma once

#include "Core.h"


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
