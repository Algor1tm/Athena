#pragma once

#include "Athena/Core/Core.h"


namespace Athena
{
	enum class ShaderResourceType
	{
		Texture2D = 1,
		StorageTexture2D,
		UniformBuffer,
		StorageBuffer,
	};

	class ATHENA_API ShaderResource: public RefCounted
	{
	public:
		virtual ~ShaderResource() = default;
		virtual ShaderResourceType GetResourceType() = 0;
	};
}
