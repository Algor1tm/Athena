#pragma once

#include "Athena/Core/Core.h"


namespace Athena
{
	enum class RenderResourceType
	{
		Texture2D = 1,
		TextureCube,
		Sampler,
		UniformBuffer,
		StorageBuffer,
	};

	class ATHENA_API RenderResource: public RefCounted
	{
	public:
		virtual ~RenderResource() = default;
		virtual RenderResourceType GetResourceType() = 0;
	};
}
