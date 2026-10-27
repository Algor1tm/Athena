#include "Format.h"
#include "Athena/Renderer/RenderThread.h"
#include "Athena/Renderer/RHI/RHI.h"


namespace Athena
{
	//	Format						  Name				Color  Depth Stencil Float  Srgb  bpp  NumComponents
	static std::unordered_map<Format, FormatProperties> s_FormatProperties =
	{
		{ Format::Unknown,			{ "Unknown",		 false, false, false, false, false, 0, 0 } },
		{ Format::R8,				{ "R8",				 true,  false, false, false, false, 1, 1 } },
		{ Format::RG8,				{ "RG8",			 true,  false, false, false, false, 2, 2 } },
		{ Format::RGBA8,			{ "RGBA8",			 true,  false, false, false, false, 4, 4 } },
		{ Format::RGBA8_SRGB,		{ "RGBA8_SRGB",		 true,  false, false, false, true,  4, 4 } },
		{ Format::R10G10B10A2,		{ "R10G10B10A2",	 true,  false, false, false, false, 4, 4 } },
															    
		{ Format::R32F,				{ "R32F",			 true,  false, false, true,  false, 4, 1 } },
		{ Format::RG16F,			{ "RG16F",			 true,  false, false, true,  false, 4, 2 } },
		{ Format::RG32F,			{ "RG32F",			 true,  false, false, true,  false, 8, 2 } },
		{ Format::R11G11B10UF,		{ "R11G11B10UF",	 true,  false, false, true,  false, 4, 3 } },
		{ Format::RGB32F,			{ "RGBA32F",		 true,  false, false, true,  false, 16, 4 } },
		{ Format::RGBA32UInt,		{ "RGBA32UInt",		 true,  false, false, false, false, 16, 4 } },

		{ Format::DEPTH16,			{ "DEPTH16",		 false, true,  false, false, false, 2, 1 } },
		{ Format::DEPTH24STENCIL8,	{ "DEPTH24STENCIL8", false, true,  true,  false, false, 4, 2 } },
		{ Format::DEPTH32F,			{ "DEPTH32F",		 false, true,  false, true,  false, 4, 1 } },
	};


	const FormatProperties& GetFormatProperties(Format format)
	{
		return s_FormatProperties[format];
	}

	const FormatSupportProperties& GetFormatSupportProperties(Format format)
	{
		if (RenderThread::Get().IsRHIInitialized())
		{
			return RHI::Get()->GetFormatSupportProperties(format);
		}

		static FormatSupportProperties s_Dummy = { true, true, true };
		return s_Dummy;
	}
}
