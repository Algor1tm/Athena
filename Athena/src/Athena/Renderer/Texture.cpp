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

	SubTexture2D::SubTexture2D(const Ref<Texture2D>& texture, const Vector2& min, const Vector2& max)
		: m_Texture(texture)
	{
		m_TexCoords[0] = { min.x, min.y };
		m_TexCoords[1] = { max.x, min.y };
		m_TexCoords[2] = { max.x, max.y };
		m_TexCoords[3] = { min.x, max.y };
	}

	Ref<SubTexture2D> SubTexture2D::CreateFromCoords(const Ref<Texture2D>& texture, const Vector2& coords, const Vector2& cellSize, const Vector2& spriteSize)
	{
		Vector2 min = { coords.x * cellSize.x / texture->GetWidth(), coords.y * cellSize.y / texture->GetHeight() };
		Vector2 max = { (coords.x + spriteSize.x) * cellSize.x / texture->GetWidth(), (coords.y + spriteSize.y) * cellSize.y / texture->GetHeight() };

		return CreateRef<SubTexture2D>(texture, min, max);
	}


	Texture2DInstance::Texture2DInstance(const Ref<Texture2D>& texture)
	{
		Set(texture);
	}

	Texture2DInstance::Texture2DInstance(const Ref<SubTexture2D>& subtexture)
	{
		Set(subtexture);
	}

	void Texture2DInstance::Set(const Ref<Texture2D>& texture)
	{
		m_Texture = texture;
		m_TexCoords = { Vector2{0.f, 0.f}, {1.f, 0.f}, {1.f, 1.f}, {0.f, 1.f} };
	}

	void Texture2DInstance::Set(const Ref<SubTexture2D>& subtexture)
	{
		m_Texture = subtexture->GetNativeTexture();
		m_TexCoords = subtexture->GetTexCoords();
	}
}
