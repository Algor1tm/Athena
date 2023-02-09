#include "Texture.h"

#include "Athena/Core/FileSystem.h"

#include "Athena/Renderer/Renderer.h"

#include "Athena/Platform/OpenGL/GLTexture2D.h"
#include "Athena/Platform/Direct3D/D3D11Texture2D.h"

#include "Athena/Platform/OpenGL/GLCubemap.h"


namespace Athena
{
	Ref<Texture2D> Texture2D::m_WhiteTexture = nullptr;

	Ref<Texture2D> Texture2D::Create(const Texture2DDescription& desc)
	{
		if(!desc.TexturePath.empty())
			ATN_CORE_ASSERT(FileSystem::Exists(desc.TexturePath), "Invalid filepath for Texture2D!");

		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			return CreateRef<GLTexture2D>(desc); break;
		case RendererAPI::API::Direct3D:
			return CreateRef<D3D11Texture2D>(desc); break;
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
			Texture2DDescription desc;
			desc.Width = 1;
			desc.Height = 1;
			desc.Format = TextureFormat::RGBA8;

			m_WhiteTexture = Texture2D::Create(desc);
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

	Ref<Cubemap> Cubemap::Create(const CubemapDescription& desc)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			return CreateRef<GLCubemap>(desc); break;
		//case RendererAPI::API::Direct3D:
		//	return CreateRef<D3D11Cubemap>(path); break;
		case RendererAPI::API::None:
			ATN_CORE_ASSERT(false, "Renderer API None is not supported");
		}

		ATN_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}
}
