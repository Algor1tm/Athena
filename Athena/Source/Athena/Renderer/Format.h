#pragma once

#include "Athena/Core/Core.h"


namespace Athena
{
	// If type is not specified it is UNORM
	enum Format
	{
		Unknown = 0,
		// Color
		R8,
		RG8,
		RGBA8,
		RGBA8_SRGB,
		R10G10B10A2,

		R32F,
		RG16F,
		RG32F,
		R11G11B10UF,
		RGB32F,
		RGBA16F,
		RGBA32F,
		RGBA32UInt,

		//Depth/Stencil
		DEPTH16,
		DEPTH24STENCIL8,
		DEPTH32F,

		FormatNum
	};

	struct FormatProperties
	{
		const char* Name;
		bool HasColor;
		bool HasDepth;
		bool HasStencil;
		bool IsFloat;
		bool IsSRGB;
		uint32 BytesPerPixel;
		uint32 NumComponents;
	};

	struct FormatSupportProperties
	{
		bool IsTextureSupported;
		bool IsRenderTargetSupported;
		bool IsVertexBufferSupported;
	};

	ATHENA_API const FormatProperties& GetFormatProperties(Format format);
	ATHENA_API const FormatSupportProperties& GetFormatSupportProperties(Format format);
}
