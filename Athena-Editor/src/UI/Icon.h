#pragma once

#include "Athena/Renderer/Texture.h"

#include <filesystem>


namespace Athena::UI
{
	class Icon
	{
	public:
		Icon(const std::filesystem::path& path)
		{
			m_Texture = Texture2D::Create(path.string());
			m_TextureID = m_Texture->GetRendererID();
		}

		void* GetRendererIDvoid() { return reinterpret_cast<void*>((uint64)m_TextureID); }
		RendererID GetRendererID() { return m_TextureID; }

	private:
		Ref<Texture2D> m_Texture;
		RendererID m_TextureID;
	};
}
