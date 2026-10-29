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

	class D3D12TextureView : public TextureView
	{
	public:
		D3D12TextureView(const TextureViewCreateInfo& info, Texture* texture);
		virtual ~D3D12TextureView();

		virtual void Init() override;
		virtual void Invalidate() override;

		D3D12Texture* GetD3D12Texture() const;
		D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle() const { return m_Descriptor; }

	private:
		void CleanUp();

	private:
		String m_Name;
		D3D12_CPU_DESCRIPTOR_HANDLE m_Descriptor;
	};
}
