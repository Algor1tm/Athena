#pragma once

#include "Athena/Core/Core.h"


namespace Athena
{
	enum Format
	{
		Unknown = 0,
		// Color
		R8,
		R8_SRGB,
		RG8,
		RG8_SRGB,
		RGB8,
		RGB8_SRGB,
		RGBA8,
		RGBA8_SRGB,

		R32F,
		RG16F,
		RGB16F,
		R11G11B10F,
		RGB32F,
		RGBA16F,
		RGBA32F,

		//Depth/Stencil
		DEPTH16,
		DEPTH24STENCIL8,
		DEPTH32F,

		FormatNum
	};

	struct FormatProperties
	{
		const char* Name;
		bool IsColor;
		bool IsDepth;
		bool IsStencil;
		bool IsFloat;
		bool IsSRGB;
		uint32 BytesPerPixel;
		uint32 NumComponents;
		bool IsTextureSupported;
		bool IsRenderTargetSupported;
		bool IsBufferSupported;
	};
}
