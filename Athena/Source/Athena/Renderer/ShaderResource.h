#pragma once

#include "Athena/Core/Core.h"


namespace Athena
{
	enum class ShaderResourceType
	{
		UniformBuffer = 1
	};

	class ATHENA_API ShaderResource: public RefCounted
	{
	public:
		virtual ShaderResourceType GetResourceType() = 0;
	};
}
