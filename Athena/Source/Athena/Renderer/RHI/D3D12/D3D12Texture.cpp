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

        static D3D12_SHADER_COMPONENT_MAPPING TranslateMapping(ComponentMapping swizzle, UINT identity)
        {
            switch (swizzle)
            {
            case ComponentMapping::R: return D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0;
            case ComponentMapping::G: return D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1;
            case ComponentMapping::B: return D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_2;
            case ComponentMapping::A: return D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_3;
            case ComponentMapping::ZERO: return D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0;
            case ComponentMapping::ONE: return D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_1;
            case ComponentMapping::IDENTITY: return (D3D12_SHADER_COMPONENT_MAPPING)identity;
            }

            checkf(false);
            return (D3D12_SHADER_COMPONENT_MAPPING)identity;
        }

        static uint32 GetLayerMultiplier(TextureType type)
        {
            if (type == TextureType::TEXTURE_CUBE)
                return 6;

            return 1;
        }

        static const char* ViewTypeToString(TextureViewType type)
        {
            switch (type)
            {
            case TextureViewType::SRV: return "SRV";
            case TextureViewType::UAV: return "UAV";
            case TextureViewType::RTV: return "RTV";
            case TextureViewType::DSV: return "DSV";
            case TextureViewType::DSV_READONLY: return "DSV";
            }

            checkf(false);
            return "Unknown";
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
        textureDesc.DepthOrArraySize = GetInfo().Layers * D3D12::GetLayerMultiplier(GetType());
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

        sizeof(D3D12_UNORDERED_ACCESS_VIEW_DESC);
        sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC);
        sizeof(D3D12_RENDER_TARGET_VIEW_DESC);
        sizeof(D3D12_DEPTH_STENCIL_VIEW_DESC);
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

    D3D12TextureView::D3D12TextureView(const TextureViewCreateInfo& info, Texture* texture)
        : TextureView(info, texture)
    {
        TRACY_PROFILE_FUNC();

        if (GetInfo().Name.empty())
        {
            const auto& info = GetInfo();

            m_Name = fmt::format("{}_{}_Mip[{},{}]_Layer[{},{}]", GetTexture()->GetName(), D3D12::ViewTypeToString(GetViewType()),
                info.Range.BaseMipLevel, info.Range.BaseMipLevel + info.Range.MipLevelCount - 1,
                info.Range.BaseLayer, info.Range.BaseLayer + info.Range.LayerCount - 1);
        }
        else
        {
            m_Name = GetInfo().Name;
        }
    }

    D3D12TextureView::~D3D12TextureView()
    {
        CleanUp();
    }

    void D3D12TextureView::Init()
    {
        TRACY_PROFILE_FUNC();

        Invalidate();
    }

    void D3D12TextureView::Invalidate()
    {
        auto& info = GetInfo();

        DXGI_FORMAT format = D3D12::TranslateFormat(GetTexture()->GetFormat());

        TextureType textureType = GetTextureType();
        uint32 layerCount = GetRange().LayerCount;
        uint32 baseLayer = GetRange().BaseLayer;

        // TODO: texture Aspects
        if (GetViewType() == TextureViewType::SRV)
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = format;

            D3D12_SHADER_COMPONENT_MAPPING r = D3D12::TranslateMapping(info.Swizzle.R, 0);
            D3D12_SHADER_COMPONENT_MAPPING g = D3D12::TranslateMapping(info.Swizzle.G, 1);
            D3D12_SHADER_COMPONENT_MAPPING b = D3D12::TranslateMapping(info.Swizzle.B, 2);
            D3D12_SHADER_COMPONENT_MAPPING a = D3D12::TranslateMapping(info.Swizzle.A, 3);
            srvDesc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(r, g, b, a);
            
            if (textureType == TextureType::TEXTURE_2D)
            {
                if (layerCount > 1 || baseLayer > 0)
                {
                    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                    srvDesc.Texture2DArray.MostDetailedMip = GetRange().BaseMipLevel;
                    srvDesc.Texture2DArray.MipLevels = GetRange().MipLevelCount;
                    srvDesc.Texture2DArray.FirstArraySlice = GetRange().BaseLayer;
                    srvDesc.Texture2DArray.ArraySize = GetRange().LayerCount;
                    srvDesc.Texture2DArray.PlaneSlice = 0;
                    srvDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
                }
                else
                {
                    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                    srvDesc.Texture2D.MostDetailedMip = GetRange().BaseMipLevel;
                    srvDesc.Texture2D.MipLevels = GetRange().MipLevelCount;
                    srvDesc.Texture2D.PlaneSlice = 0;
                    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
                }
            }
            else
            {
                if (layerCount > 6 && layerCount )
                {
                    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
                    srvDesc.TextureCubeArray.MostDetailedMip = GetRange().BaseMipLevel;
                    srvDesc.TextureCubeArray.MipLevels = GetRange().MipLevelCount;  
                    srvDesc.TextureCubeArray.First2DArrayFace = GetRange().BaseLayer * 6;
                    srvDesc.TextureCubeArray.NumCubes = GetRange().LayerCount;
                    srvDesc.TextureCubeArray.ResourceMinLODClamp = 0.0f;
                }
                else
                {
                    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
                    srvDesc.TextureCube.MostDetailedMip = GetRange().BaseMipLevel;
                    srvDesc.TextureCube.MipLevels = GetRange().MipLevelCount;
                    srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
                }
            }


        }
        else if (GetViewType() == TextureViewType::UAV)
        {
            ensuref(GetTextureType() != TextureType::TEXTURE_CUBE);
            ensuref(GetRange().MipLevelCount == 1);

            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.Format = format;

            if (layerCount > 1 || baseLayer > 0)
            {
                uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
                uavDesc.Texture2DArray.MipSlice = GetRange().BaseMipLevel;
                uavDesc.Texture2DArray.FirstArraySlice = GetRange().BaseLayer;
                uavDesc.Texture2DArray.ArraySize = GetRange().LayerCount;
                uavDesc.Texture2DArray.PlaneSlice = 0;
            }
            else
            {
                uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
                uavDesc.Texture2D.MipSlice = GetRange().BaseMipLevel;
                uavDesc.Texture2D.PlaneSlice = 0;
            }
        }
        else if (GetViewType() == TextureViewType::RTV)
        {
            ensuref(GetTextureType() != TextureType::TEXTURE_CUBE);
            ensuref(GetRange().MipLevelCount == 1);

            D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
            rtvDesc.Format = format;

            if (layerCount > 1 || baseLayer > 0)
            {
                rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
                rtvDesc.Texture2DArray.MipSlice = GetRange().BaseMipLevel;
                rtvDesc.Texture2DArray.FirstArraySlice = GetRange().BaseLayer;
                rtvDesc.Texture2DArray.ArraySize = GetRange().LayerCount;
                rtvDesc.Texture2DArray.PlaneSlice = 0;
            }
            else
            {
                rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
                rtvDesc.Texture2D.MipSlice = GetRange().BaseMipLevel;
                rtvDesc.Texture2D.PlaneSlice = 0;
            }
        }
        else if (GetViewType() == TextureViewType::DSV || GetViewType() == TextureViewType::DSV_READONLY)
        {
            D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
            dsvDesc.Format = format;
            dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

            if (GetViewType() == TextureViewType::DSV_READONLY)
                dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH | D3D12_DSV_FLAG_READ_ONLY_STENCIL;

            if (layerCount > 1 || baseLayer > 0)
            {
                dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
                dsvDesc.Texture2DArray.MipSlice = GetRange().BaseMipLevel;
                dsvDesc.Texture2DArray.FirstArraySlice = GetRange().BaseLayer;
                dsvDesc.Texture2DArray.ArraySize = GetRange().LayerCount;
            }
            else
            {
                dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
                dsvDesc.Texture2D.MipSlice = GetRange().BaseMipLevel;
            }
        }
    }

    D3D12Texture* D3D12TextureView::GetD3D12Texture() const
    {
        return (D3D12Texture*)GetTexture();
    }

    void D3D12TextureView::CleanUp()
    {

    }
}
