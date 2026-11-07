#pragma once 

#include "Athena/Core/Core.h"


namespace Athena
{
	enum class ResourceState
	{
		Init = 0,

		// Read states
		CopySrc = 1,
		VertexInput = 2,
		SRVNonPixelShader = 4,
		SRVPixelShader = 5,
		SRVGraphics = 6,
		SRVCompute = 7,
		SRVAll = 8,
		DSVRead = 9,

		// Read-Write states
		CopyDst = 10,
		UAV = 11,
		RTV = 12,
		DSVReadWrite = 13,
	};

}
