#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Texture.h"
#include "Athena/Renderer/RHI/D3D12/D3D12RHI.h"


namespace Athena
{
	class D3D12Texture : public Texture
	{
	public:
		D3D12Texture(const TextureCreateInfo& info);
		virtual ~D3D12Texture();

		virtual void Init() override;
		virtual void Resize(uint32 width, uint32 height) override;
		virtual void Upload(const Buffer& srcBuffer, const TextureRegion& region) override;
		virtual void UploadAndBlitMipMap(const Buffer& srcBuffer) override;

	private:
		void CleanUp();

	private:
		D3D12Resource m_TextureResource;
	};
}
