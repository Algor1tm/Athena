#include "atnpch.h"
#include "Texture.h"

#include "Athena/Renderer/Renderer.h"
#include "Athena/Platform/OpenGL/OpenGLTexture2D.h"


namespace Athena
{
	Ref<Texture2D> Texture2D::Create(uint32 width, uint32 height)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			return CreateRef<OpenGLTexture2D>(width, height); break;
		case RendererAPI::API::Direct3D:
			ATN_CORE_ASSERT(false, "Renderer API Direct3D is not supported"); break;
		case RendererAPI::API::None:
			ATN_CORE_ASSERT(false, "Renderer API None is not supported");
		}

		ATN_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	Ref<Texture2D> Texture2D::Create(const String& path)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			return CreateRef<OpenGLTexture2D>(path); break;
		case RendererAPI::API::Direct3D:
			ATN_CORE_ASSERT(false, "Renderer API Direct3D is not supported"); break;
		case RendererAPI::API::None:
			ATN_CORE_ASSERT(false, "Renderer API None is not supported");
		}

		ATN_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}
}
