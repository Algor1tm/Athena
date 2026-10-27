#include "D3D12Texture.h"

#include "Athena/Renderer/RHI/D3D12/D3D12Utils.h"


namespace Athena
{
    namespace D3D12
    {
        static D3D12_RESOURCE_FLAGS TranslateImageUsage(TextureUsage usage, Format format)
        {
            auto& formatProperties = GetFormatProperties(format);
            bool depthStencil = formatProperties.HasDepth || formatProperties.HasStencil;

            D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;

            // TextureUsage::SAMPLED is enabled by default unlike in Vulkan

            if (usage & TextureUsage::UAV)
                flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

            if (usage & TextureUsage::ATTACHMENT)
                flags |= depthStencil ? D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL : D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

            return flags;
        }

        static D3D12_CLEAR_VALUE GetDefaultClearValue(Format format)
        {
            auto& formatProperties = GetFormatProperties(format);

            D3D12_CLEAR_VALUE result;
            result.Format = D3D12::TranslateFormat(format);

            if (formatProperties.HasColor)
            {
                result.Color[0] = 0.f;
                result.Color[1] = 0.f;
                result.Color[2] = 0.f;
                result.Color[3] = 1.f;
            }
            else
            {
                result.DepthStencil.Depth = 0.f;
                result.DepthStencil.Stencil = 1.f;
            }

            return result;
        } 
    }
    
	D3D12Texture::D3D12Texture(const TextureCreateInfo& info)
		: Texture(info)
	{

	}

	D3D12Texture::~D3D12Texture()
	{
		CleanUp();
	}

    void D3D12Texture::CleanUp()
    {
        if (m_TextureResource)
        {
            D3D12RHI::Get()->DeleteFrameResourcesDeferred([texture = m_TextureResource]() mutable
                {
                    texture.Release();
                });
        }
    }

	void D3D12Texture::Init()
	{
		TRACY_PROFILE_FUNC();

		Resize(GetWidth(), GetHeight());
	}

	void D3D12Texture::Resize(uint32 width, uint32 height)
	{
        TRACY_PROFILE_FUNC();

        Texture::Resize(width, height);

        CleanUp();

        D3D12_RESOURCE_DESC textureDesc = {};
        textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        textureDesc.Alignment = 0;
        textureDesc.Width = width;
        textureDesc.Height = height;
        textureDesc.DepthOrArraySize = GetInfo().Layers;
        textureDesc.MipLevels = GetMipLevelsCount();
        textureDesc.Format = D3D12::TranslateFormat(GetFormat());
        textureDesc.SampleDesc.Count = GetInfo().Samples; 
        textureDesc.SampleDesc.Quality = 0;
        textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        textureDesc.Flags = D3D12::TranslateImageUsage(GetInfo().Usage, GetFormat());

        D3D12_CLEAR_VALUE clearValue = D3D12::GetDefaultClearValue(GetFormat());
        D3D12_CLEAR_VALUE* clearValuePTR = (GetInfo().Usage & TextureUsage::ATTACHMENT) ? &clearValue : nullptr;

        m_TextureResource = D3D12RHI::Get()->GetAllocator()->AllocateResource(textureDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_HEAP_TYPE_DEFAULT, clearValuePTR);
        D3D12_SET_DEBUG_NAME(m_TextureResource.GetResource(), fmt::format("Texture_{}", GetName()));

        InvalidateViews();
	}

	void D3D12Texture::Upload(const Buffer& srcBuffer, const TextureRegion& region)
	{
		ensuref(false);
	}

	void D3D12Texture::UploadAndBlitMipMap(const Buffer& srcBuffer)
	{
        uint64 imageSize = GetWidth() * GetHeight() * GetFormatProperties(GetFormat()).BytesPerPixel;
        ensure(srcBuffer.Size() <= imageSize, "Texture upload buffer is too small!");

        D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
        UINT numRows = 0;
        UINT64 rowSize = 0;
        UINT64 uploadSize = 0;

        D3D12_RESOURCE_DESC textureDesc = m_TextureResource.GetResource()->GetDesc();

        D3D12RHI::Get()->GetDevice()->GetCopyableFootprints(
            &textureDesc,
            0,
            1,
            0,
            &footprint,
            &numRows,
            &rowSize,
            &uploadSize);

        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width = uploadSize;
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.SampleDesc.Count = 1;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Flags = D3D12_RESOURCE_FLAG_NONE;

        D3D12Resource stagingBuffer = D3D12RHI::Get()->GetAllocator()->AllocateResource(desc, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_HEAP_TYPE_UPLOAD);
        void* mapped = stagingBuffer.MapMemory();

        const byte* src = srcBuffer.Data();
        byte* dst = (byte*)mapped + footprint.Offset;

        const UINT sourceRowPitch = GetWidth() * GetFormatProperties(GetFormat()).BytesPerPixel;

        for (UINT y = 0; y < GetHeight(); ++y)
        {
            memcpy(dst + y * footprint.Footprint.RowPitch, src + y * sourceRowPitch, sourceRowPitch);
        }

        stagingBuffer.UnmapMemory();
        D3D12CommandList* list = D3D12RHI::Get()->GetCmdListManager()->GetTransferCommandList();

        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = m_TextureResource.GetResource();
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;

        list->CommandList->ResourceBarrier(1, &barrier);

        D3D12_TEXTURE_COPY_LOCATION dstLocation{};
        dstLocation.pResource = m_TextureResource.GetResource();
        dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dstLocation.SubresourceIndex = 0;

        D3D12_TEXTURE_COPY_LOCATION srcLocation{};
        srcLocation.pResource = stagingBuffer.GetResource();
        srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        srcLocation.PlacedFootprint = footprint;

        list->CommandList->CopyTextureRegion(
            &dstLocation, 0, 0, 0,
            &srcLocation, nullptr);

        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

        list->CommandList->ResourceBarrier(1, &barrier);

        list->AddCallbackOnCompletion([stagingBuffer]() mutable
            {
                stagingBuffer.Release();
            });
	}
}
