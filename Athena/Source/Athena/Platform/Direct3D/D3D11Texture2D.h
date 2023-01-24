#pragma once

#include "Athena/Renderer/Texture.h"

#include "D3D11GraphicsContext.h"


namespace Athena
{
	class ATHENA_API D3D11Texture2D : public Texture2D
	{
	public:
		D3D11Texture2D(uint32 width, uint32 height);
		D3D11Texture2D(const Texture2DDescription& desc);
		virtual ~D3D11Texture2D();

		virtual inline uint32 GetWidth() const override { return m_Width; }
		virtual inline uint32 GetHeight() const override { return m_Height; }

		virtual void SetData(const void* data, uint32 size) override;

		virtual void Bind(uint32 slot = 0) const override;
		virtual bool IsLoaded() const override { return m_IsLoaded; };

		virtual const Filepath& GetFilepath() const override { return m_Path; };

	private:
		Microsoft::WRL::ComPtr<ID3D11Texture2D> m_Texture2D;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_ShaderResourceView;

		Filepath m_Path;
		uint32 m_Width = 0, m_Height = 0;
		bool m_IsLoaded = false;
	};
}
