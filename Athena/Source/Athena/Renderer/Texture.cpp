#include "atnpch.h"
#include "Texture.h"

#include "Athena/Renderer/Renderer.h"
#include "Athena/Platform/OpenGL/GLTexture2D.h"
#include "Athena/Platform/Direct3D/D3D11Texture2D.h"


namespace Athena
{
	Ref<Texture2D> Texture2D::m_WhiteTexture = nullptr;


	Ref<Texture2D> Texture2D::Create(uint32 width, uint32 height)
	{
		ATN_CORE_ASSERT(width > 0 && height > 0, "Invalid size for Texture2D!");

		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			return CreateRef<GLTexture2D>(width, height); break;
		case RendererAPI::API::Direct3D:
			return CreateRef<D3D11Texture2D>(width, height); break;
		case RendererAPI::API::None:
			ATN_CORE_ASSERT(false, "Renderer API None is not supported");
		}

		ATN_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	Ref<Texture2D> Texture2D::Create(const Filepath& path)
	{
		ATN_CORE_ASSERT(std::filesystem::exists(path), "Invalid filepath for Texture2D!");

		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			return CreateRef<GLTexture2D>(path); break;
		case RendererAPI::API::Direct3D:
			return CreateRef<D3D11Texture2D>(path); break;
		case RendererAPI::API::None:
			ATN_CORE_ASSERT(false, "Renderer API None is not supported");
		}

		ATN_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	Ref<Texture2D> Texture2D::WhiteTexture()
	{
		if (m_WhiteTexture == nullptr)
		{
			m_WhiteTexture = Texture2D::Create(1, 1);
			uint32 whiteTextureData = 0xffffffff;
			m_WhiteTexture->SetData(&whiteTextureData, sizeof(uint32));
		}

		return m_WhiteTexture;
	}

	Texture2DInstance::Texture2DInstance() 
	{
		SetTexCoords({ Vector2{0.f, 0.f}, {1.f, 0.f}, {1.f, 1.f}, {0.f, 1.f} });
	}

	Texture2DInstance::Texture2DInstance(const Ref<Texture2D>& texture)
	{
		SetTexture(texture);
		SetTexCoords({ Vector2{0.f, 0.f}, {1.f, 0.f}, {1.f, 1.f}, {0.f, 1.f} });
	}

	Texture2DInstance::Texture2DInstance(const Ref<Texture2D>& texture, const std::array<Vector2, 4>& texCoords)
	{
		SetTexture(texture);
		SetTexCoords(texCoords);
	}

	Texture2DInstance::Texture2DInstance(const Ref<Texture2D>& texture, const Vector2& min, const Vector2& max)
	{
		SetTexture(texture);
		SetTexCoords(min, max);
	}

	void Texture2DInstance::SetTexCoords(const Vector2& min, const Vector2& max) 
	{
		float width = (float)m_Texture->GetWidth();
		float height = (float)m_Texture->GetHeight();

		m_TexCoords[0] = { min.x / width, min.y / height };
		m_TexCoords[1] = { max.x / width, min.y / height };
		m_TexCoords[2] = { max.x / width, max.y / height };
		m_TexCoords[3] = { min.x / width, max.y / height };
	}
}
