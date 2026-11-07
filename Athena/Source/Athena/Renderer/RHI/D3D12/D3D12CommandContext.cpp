#include "D3D12CommandContext.h"

#include "Athena/Renderer/RHI/D3D12/D3D12RHI.h"
#include "Athena/Renderer/RHI/D3D12/D3D12Texture.h"
#include "Athena/Renderer/RHI/D3D12/D3D12Buffers.h"
#include "Athena/Renderer/RHI/D3D12/D3D12Utils.h"


namespace Athena
{
	enum class BarrierType
	{
		Transition,
		TransferDstRelease,
		Acquire
	};

	namespace D3D12
	{
		static D3D12_COMMAND_LIST_TYPE TranslateCommandContextType(CommandContextType type)
		{
			switch (type)
			{
			case CommandContextType::Graphics:		return D3D12_COMMAND_LIST_TYPE_DIRECT;
			case CommandContextType::AsyncCompute:  return D3D12_COMMAND_LIST_TYPE_COMPUTE;
			case CommandContextType::AsyncTransfer: return D3D12_COMMAND_LIST_TYPE_COPY;
			}

			checkf(false);
			return D3D12_COMMAND_LIST_TYPE_DIRECT;
		}

		static D3D12_RESOURCE_STATES TranslateStateBuffer(ResourceState state, GPUBufferType type)
		{
			switch (state)
			{
			case ResourceState::Init:				return D3D12_RESOURCE_STATE_COMMON;
			case ResourceState::CopySrc:			return D3D12_RESOURCE_STATE_COPY_SOURCE;
			case ResourceState::VertexInput:		return D3D12_RESOURCE_STATE_COPY_SOURCE;
			case ResourceState::SRVNonPixelShader:	return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
			case ResourceState::SRVPixelShader:		return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			case ResourceState::SRVCompute:			return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
			case ResourceState::SRVGraphics:		return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			case ResourceState::SRVAll:				return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			case ResourceState::CopyDst:			return D3D12_RESOURCE_STATE_COPY_DEST;
			case ResourceState::UAV:				return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			}

			if (state == ResourceState::VertexInput)
			{
				switch (type)
				{
				case GPUBufferType::VertexBuffer: return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
				case GPUBufferType::IndexBuffer:  return D3D12_RESOURCE_STATE_INDEX_BUFFER;
				}
			}

			checkf(false);
			return D3D12_RESOURCE_STATE_COMMON;
		}

		static D3D12_RESOURCE_STATES TranslateStateTexture(ResourceState state)
		{
			switch (state)
			{
			case ResourceState::Init:				return D3D12_RESOURCE_STATE_COMMON;
			case ResourceState::CopySrc:			return D3D12_RESOURCE_STATE_COPY_SOURCE;
			case ResourceState::SRVNonPixelShader:	return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
			case ResourceState::SRVPixelShader:		return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			case ResourceState::SRVCompute:			return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
			case ResourceState::SRVGraphics:		return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			case ResourceState::SRVAll:				return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			case ResourceState::DSVRead:			return D3D12_RESOURCE_STATE_DEPTH_READ;
			case ResourceState::CopyDst:			return D3D12_RESOURCE_STATE_COPY_DEST;
			case ResourceState::UAV:				return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			case ResourceState::RTV:				return D3D12_RESOURCE_STATE_RENDER_TARGET;
			case ResourceState::DSVReadWrite:		return D3D12_RESOURCE_STATE_DEPTH_WRITE;
			}

			checkf(false);
			return D3D12_RESOURCE_STATE_COMMON;
		}

		static bool GetBufferBarrier(D3D12_RESOURCE_BARRIER* barrier, 
			GPUBuffer* buffer, 
			ResourceState src, 
			ResourceState dst, 
			BarrierType type = BarrierType::Transition, 
			CommandContextType srcContext = CommandContextType::Graphics, 
			CommandContextType dstContext = CommandContextType::Graphics)
		{
			if (src == ResourceState::UAV && dst == ResourceState::UAV && type == BarrierType::Transition)
			{
				barrier->Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
				barrier->UAV.pResource = ((D3D12GPUBuffer*)buffer)->GetResource();
				return true;
			}

			barrier->Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier->Transition.pResource = ((D3D12GPUBuffer*)buffer)->GetResource();
			barrier->Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			barrier->Transition.StateBefore = TranslateStateBuffer(src, buffer->GetType());
			barrier->Transition.StateAfter = TranslateStateBuffer(dst, buffer->GetType());

			if (type == BarrierType::TransferDstRelease)
			{
				barrier->Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
			}
			else if (type == BarrierType::Acquire)
			{
				if (srcContext == CommandContextType::AsyncTransfer || dstContext == CommandContextType::AsyncTransfer)
				{
					barrier->Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
				}
			}

			return barrier->Transition.StateBefore != barrier->Transition.StateAfter;
		}

		static bool GetTextureBarrier(D3D12_RESOURCE_BARRIER* barrier, 
			Texture* texture, 
			uint32 subresource, 
			ResourceState src, 
			ResourceState dst, 
			BarrierType type = BarrierType::Transition, 
			CommandContextType srcContext = CommandContextType::Graphics, 
			CommandContextType dstContext = CommandContextType::Graphics)
		{
			if (src == ResourceState::UAV && dst == ResourceState::UAV && type == BarrierType::Transition)
			{
				barrier->Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
				barrier->UAV.pResource = ((D3D12Texture*)texture)->GetResource();
				return true;
			}

			barrier->Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier->Transition.pResource = ((D3D12Texture*)texture)->GetResource();
			barrier->Transition.Subresource = subresource;
			barrier->Transition.StateBefore = TranslateStateTexture(src);
			barrier->Transition.StateAfter = TranslateStateTexture(dst);

			// Transfer queue always uses STAT_COMMON for resources
			if (type == BarrierType::TransferDstRelease)
			{
				barrier->Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
			}
			else if(type == BarrierType::Acquire)
			{
				if (srcContext == CommandContextType::AsyncTransfer)
				{
					barrier->Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
				}
				else if (dstContext == CommandContextType::AsyncTransfer)
				{
					barrier->Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
					barrier->Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
				}
			}

			return barrier->Transition.StateBefore != barrier->Transition.StateAfter;
		}
	}

	D3D12CommandContext::D3D12CommandContext(CommandContextType type)
		: m_Type(type)
		, m_CurrentCommandList(nullptr)
	{
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Type = D3D12::TranslateCommandContextType(GetType());
		queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.NodeMask = 0;

		D3D12_CHECK(D3D12RHI::Get()->GetDevice()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_Queue)));
		D3D12_SET_DEBUG_NAME(m_Queue, std::format("{}_Queue", EnumUtils::CmdContextTypeToString(GetType())));

		m_Fence.Init();
		D3D12_SET_DEBUG_NAME(m_Fence.Fence, std::format("{}_Fence", EnumUtils::CmdContextTypeToString(GetType())));

#if ATN_ENABLE_TRACY_GPU
		InitTracy();
#endif
	}

	D3D12CommandContext::~D3D12CommandContext()
	{
#if ATN_ENABLE_TRACY_GPU
		if (m_TracyContext)
			TracyD3D12Destroy(m_TracyContext);
#endif

		for (uint32 i = 0; i < m_CommandListsArray.size(); ++i)
		{
			D3D12CommandList* list = m_CommandListsArray[i];
			list->WaitCompletion();
			delete list;
		}

		m_Fence.Destroy();
		m_Queue->Release();
	}

	void D3D12CommandContext::BeginFrame()
	{
		TRACY_PROFILE_FUNC();

		UpdateCommandLists();
	}

	void D3D12CommandContext::EndFrame()
	{ 
		TRACY_PROFILE_FUNC();

		Flush();
	}

	void D3D12CommandContext::Drain(CommandContextType srcContextType, ResourceState waitDstStage)
	{
		TRACY_PROFILE_FUNC();

		Ref<D3D12CommandContext> srcContext = RHI::Get()->GetContext(srcContextType).As<D3D12CommandContext>();
		if (srcContext.Raw() == this)
		{
			return;
		}

		srcContext->Flush();

		uint64 lastSignaledValue = srcContext->m_Fence.GetSignaledValue();
		bool anyPendingCommands = srcContext->m_Fence.GetCompletedValue() == lastSignaledValue;

		// Nothing to wait on
		if (!anyPendingCommands)
			return;

		ContextSyncPoint& syncPoint = m_SyncPoints[srcContextType];
		syncPoint.Fence = srcContext->m_Fence.Fence;
		syncPoint.WaitValue = lastSignaledValue;
	}

	void D3D12CommandContext::Flush()
	{
		TRACY_PROFILE_FUNC();

		if (m_CurrentCommandList)
		{
			GPU_TRACY_END(this);

#if ATN_ENABLE_TRACY_GPU
			ensuref(m_TracyZonesStack.empty());
#endif

			for (const auto& [type, syncPoint] : m_SyncPoints)
			{
				if (syncPoint.Fence)
					m_Queue->Wait(syncPoint.Fence, syncPoint.WaitValue);
			}

#if ATN_ENABLE_TRACY_GPU
			TracyCollect();
#endif
			m_CurrentCommandList->End();
			m_CurrentCommandList->Execute(m_Queue);

			m_Fence.Signal(m_Queue);

			m_CurrentCommandList = nullptr;
		}

		// Add resources acquire barriers
		for (const auto& [type, syncPoint] : m_SyncPoints)
		{
			if (!HasPendingBarriers(type))
				continue;

			D3D12CommandList* list = GetCommandList();
			list->CmdList->ResourceBarrier(syncPoint.Barriers.size(), syncPoint.Barriers.data());
		}

		m_SyncPoints.clear();
	}

	void D3D12CommandContext::FlushAndWait()
	{
		TRACY_PROFILE_FUNC();

		if (!m_CurrentCommandList)
			return;

		D3D12CommandList* list = GetCommandList();
		Flush();
		list->WaitCompletion();
	}

	void D3D12CommandContext::Transition(Texture* texture, const TextureSubresourceRange& range, ResourceState dst)
	{
		// Copy queue does not support transitions
		if (GetType() == CommandContextType::AsyncTransfer)
			return;

		D3D12CommandList* list = GetCommandList();

		if (texture->GetFullSubresourceRange() == range && texture->AreSubresourcesSameState(range))
		{
			ResourceState src = texture->GetSubresState({ range.BaseMipLevel, range.BaseLayer, TexturePlane(range.GetFirstPlane()) });

			D3D12_RESOURCE_BARRIER barrier = {};
			if(D3D12::GetTextureBarrier(&barrier, texture, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, src, dst))
				list->CmdList->ResourceBarrier(1, &barrier);
		}
		else
		{
			uint32 firstPlane = range.GetFirstPlane();
			uint32 lastPlane = range.GetLastPlane(texture->GetPlaneCount());

			std::vector<D3D12_RESOURCE_BARRIER> barriers;
			barriers.reserve((lastPlane - firstPlane) * (range.LayerCount - range.BaseLayer) * (range.MipLevelCount - range.BaseMipLevel));

			for (uint32 plane = firstPlane; plane < lastPlane; ++plane)
			{
				for (uint32 layer = range.BaseLayer; layer < range.LayerCount; ++layer)
				{
					for (uint32 mip = range.BaseMipLevel; mip < range.MipLevelCount; ++mip)
					{
						const uint32 subresource = D3D12::CalcSubresource(mip, layer, plane, texture->GetMipCount(), texture->GetLayerCount());
						ResourceState src = texture->GetSubresState({ mip, layer, TexturePlane(plane)});

						D3D12_RESOURCE_BARRIER barrier = {};
						if (D3D12::GetTextureBarrier(&barrier, texture, subresource, src, dst))
							barriers.push_back(barrier);
					}
				}
			}

			if(!barriers.empty())
				list->CmdList->ResourceBarrier(barriers.size(), barriers.data());
		}

		texture->SetSubresState(dst, range);
	}

	void D3D12CommandContext::Transition(GPUBuffer* buffer, ResourceState dst)
	{
		// Copy queue does not support transitions
		if (GetType() == CommandContextType::AsyncTransfer)
			return;

		D3D12CommandList* list = GetCommandList();

		D3D12_RESOURCE_BARRIER barrier = {};
		if (D3D12::GetBufferBarrier(&barrier, buffer, buffer->GetState(), dst))
			list->CmdList->ResourceBarrier(1, &barrier);

		buffer->SetState(dst);
	}

	void D3D12CommandContext::TransitionOwnership(Texture* texture, const TextureSubresourceRange& range, CommandContextType dstContextType, ResourceState dst)
	{
		Ref<D3D12CommandContext> dstContext = RHI::Get()->GetContext(dstContextType).As<D3D12CommandContext>();

		if (dstContext.Raw() == this)
		{
			Transition(texture, range, dst);
			return;
		}

		dstContextType = dstContext->GetType();
		D3D12CommandList* list = GetCommandList();

		if (texture->GetFullSubresourceRange() == range && texture->AreSubresourcesSameState(range))
		{
			ResourceState src = texture->GetSubresState({ range.BaseMipLevel, range.BaseLayer, TexturePlane(range.GetFirstPlane()) });

			if (dstContextType == CommandContextType::AsyncTransfer)
			{
				D3D12_RESOURCE_BARRIER barrier = {};
				if (D3D12::GetTextureBarrier(&barrier, texture, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, src, dst, BarrierType::TransferDstRelease))
					list->CmdList->ResourceBarrier(1, &barrier);
			}

			D3D12_RESOURCE_BARRIER barrier = {};
			if (D3D12::GetTextureBarrier(&barrier, texture, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, src, dst, BarrierType::Acquire, GetType(), dstContextType))
				dstContext->m_SyncPoints[GetType()].Barriers.push_back(barrier);
		}
		else
		{
			uint32 firstPlane = range.GetFirstPlane();
			uint32 lastPlane = range.GetLastPlane(texture->GetPlaneCount());

			std::vector<D3D12_RESOURCE_BARRIER> releaseBarriers;

			for (uint32 plane = firstPlane; plane < lastPlane; ++plane)
			{
				for (uint32 layer = range.BaseLayer; layer < range.LayerCount; ++layer)
				{
					for (uint32 mip = range.BaseMipLevel; mip < range.MipLevelCount; ++mip)
					{
						const uint32 subresource = D3D12::CalcSubresource(mip, layer, plane, texture->GetMipCount(), texture->GetLayerCount());
						ResourceState src = texture->GetSubresState({ mip, layer, TexturePlane(plane) });

						if (dstContextType == CommandContextType::AsyncTransfer)
						{
							D3D12_RESOURCE_BARRIER barrier = {};
							if (D3D12::GetTextureBarrier(&barrier, texture, subresource, src, dst, BarrierType::TransferDstRelease))
								releaseBarriers.push_back(barrier);
						}

						D3D12_RESOURCE_BARRIER barrier = {};
						if (D3D12::GetTextureBarrier(&barrier, texture, subresource, src, dst, BarrierType::Acquire, GetType(), dstContextType))
							dstContext->m_SyncPoints[GetType()].Barriers.push_back(barrier);
					}
				}
			}

			if (!releaseBarriers.empty())
				list->CmdList->ResourceBarrier(releaseBarriers.size(), releaseBarriers.data());
		}

		texture->SetSubresState(dst, range);
	}

	void D3D12CommandContext::TransitionOwnership(GPUBuffer* buffer, CommandContextType dstContextType, ResourceState dst)
	{
		Ref<D3D12CommandContext> dstContext = RHI::Get()->GetContext(dstContextType).As<D3D12CommandContext>();

		if (dstContext.Raw() == this)
		{
			Transition(buffer, dst);
			return;
		}

		dstContextType = dstContext->GetType();
		D3D12CommandList* list = GetCommandList();

		if (dstContextType == CommandContextType::AsyncTransfer)
		{
			D3D12_RESOURCE_BARRIER barrier = {};
			if (D3D12::GetBufferBarrier(&barrier, buffer, buffer->GetState(), dst, BarrierType::TransferDstRelease))
				list->CmdList->ResourceBarrier(1, &barrier);
		}

		D3D12_RESOURCE_BARRIER barrier = {};
		if (D3D12::GetBufferBarrier(&barrier, buffer, buffer->GetState(), dst, BarrierType::Acquire, GetType(), dstContextType))
			dstContext->m_SyncPoints[GetType()].Barriers.push_back(barrier);
	}

	void D3D12CommandContext::UploadBuffer(GPUBuffer* buffer, const Buffer& data, uint64 offset)
	{
		TRACY_PROFILE_FUNC();

		check(buffer->GetSize() >= data.Size() + offset && data.Data() != nullptr, "Invalid initialization buffer for GPUBuffer!");

		D3D12CommandList* list = GetCommandList();

		D3D12Resource stagingBuffer = D3D12RHI::Get()->GetStagingPool()->GetBuffer(data.Size());

		void* mappedData = stagingBuffer.MapMemory({ 0, data.Size() });
		memcpy(mappedData, data.Data(), data.Size());
		stagingBuffer.UnmapMemory();

		list->CmdList->CopyBufferRegion(((D3D12GPUBuffer*)buffer)->GetResource(), offset, stagingBuffer.GetResource(), 0, data.Size());

		D3D12RHI::Get()->GetStagingPool()->MarkAsUsed(stagingBuffer);
	}

	void D3D12CommandContext::UploadTexture(Texture* texture, const Buffer& data)
	{
		uint64 imageSize = texture->GetWidth() * texture->GetHeight() * GetFormatProperties(texture->GetFormat()).BytesPerPixel;
		ensure(data.Size() <= imageSize, "Texture upload buffer is too small!");

		D3D12CommandList* list = GetCommandList();

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
		UINT numRows = 0;
		UINT64 rowSize = 0;
		UINT64 uploadSize = 0;

		ID3D12Resource* resource = ((D3D12Texture*)texture)->GetResource();
		D3D12_RESOURCE_DESC resourceDesc = resource->GetDesc();

		D3D12RHI::Get()->GetDevice()->GetCopyableFootprints(&resourceDesc, 0, 1, 0, &footprint, &numRows, &rowSize, &uploadSize);

		D3D12Resource stagingBuffer = D3D12RHI::Get()->GetStagingPool()->GetBuffer(uploadSize);
		void* mapped = stagingBuffer.MapMemory();

		const byte* src = data.Data();
		byte* dst = (byte*)mapped + footprint.Offset;

		const UINT sourceRowPitch = texture->GetWidth() * GetFormatProperties(texture->GetFormat()).BytesPerPixel;

		for (UINT y = 0; y < texture->GetHeight(); ++y)
		{
			memcpy(dst + y * footprint.Footprint.RowPitch, src + y * sourceRowPitch, sourceRowPitch);
		}

		stagingBuffer.UnmapMemory();

		D3D12_TEXTURE_COPY_LOCATION dstLocation{};
		dstLocation.pResource = resource;
		dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dstLocation.SubresourceIndex = 0;

		D3D12_TEXTURE_COPY_LOCATION srcLocation{};
		srcLocation.pResource = stagingBuffer.GetResource();
		srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		srcLocation.PlacedFootprint = footprint;

		list->CmdList->CopyTextureRegion(
			&dstLocation, 0, 0, 0,
			&srcLocation, nullptr);

		D3D12RHI::Get()->GetStagingPool()->MarkAsUsed(stagingBuffer);
	}

#if ATN_ENABLE_GPU_DEBUG
	void D3D12CommandContext::BeginMarker(const String& name)
	{
		// TODO PIX events
	}

	void D3D12CommandContext::EndMarker()
	{

	}
	void D3D12CommandContext::BeginQueueMarker(const String& name)
	{

	}

	void D3D12CommandContext::EndQueueMarker()
	{

	}

	void D3D12CommandContext::InsertMarker(const String& name)
	{

	}
#endif

#if ATN_ENABLE_TRACY_GPU
	void D3D12CommandContext::BeginTracyZone(uint32 line, std::string_view fileName, std::string_view functionName, const String& name)
	{
		if (!m_TracyContext)
			return;

		m_TracyZonesStack.push(new tracy::D3D12ZoneScope(
			m_TracyContext,
			line,
			fileName.data(), fileName.size(),
			functionName.data(), functionName.size(),
			name.data(), name.size(),
			GetCommandList()->CmdList,
			true));
	}

	void D3D12CommandContext::EndTracyZone()
	{
		if (!m_TracyContext)
			return;

		tracy::D3D12ZoneScope* zone = m_TracyZonesStack.top();
		m_TracyZonesStack.pop();

		delete zone;
	}
#endif


	D3D12CommandList* D3D12CommandContext::GetCommandList()
	{
		if (m_CurrentCommandList)
			return m_CurrentCommandList;

		for (uint32 i = 0; i < m_CommandListsArray.size(); ++i)
		{
			D3D12CommandList* list = m_CommandListsArray[i];
			if (list->State == CommandListState::Idle)
			{
				m_CurrentCommandList = list;
				break;
			}
			else if (list->State == CommandListState::Submitted)
			{
				if (list->Fence.IsCompleted())
				{
					list->OnCompletion();
					m_CurrentCommandList = list;
					break;
				}
			}
		}

		if (!m_CurrentCommandList)
		{
			m_CurrentCommandList = m_CommandListsArray.emplace_back(new D3D12CommandList(D3D12::TranslateCommandContextType(GetType())));
		}

		m_CurrentCommandList->Begin();

		D3D12_SET_DEBUG_NAME(m_CurrentCommandList->CmdList,
			std::format("{}_CommandBuffer_{}", EnumUtils::CmdContextTypeToString(GetType()), m_Fence.GetSignaledValue()));

		D3D12_SET_DEBUG_NAME(m_CurrentCommandList->Fence.Fence,
			std::format("{}_Fence_{}", EnumUtils::CmdContextTypeToString(GetType()), m_Fence.GetSignaledValue()));

		GPU_TRACY_BEGIN(this, std::format("{}Commands", EnumUtils::CmdContextTypeToString(GetType())));

		return m_CurrentCommandList;
	}

	void D3D12CommandContext::UpdateCommandLists()
	{
		TRACY_PROFILE_FUNC();

		std::vector<uint32> buffersToDelete;

		for (auto it = m_CommandListsArray.begin(); it != m_CommandListsArray.end();)
		{
			D3D12CommandList* list = *it;

			if (list->State == CommandListState::Idle)
			{
				if (Time::GetSystemCurrentTime() - list->LastUsedTime >= Time::Seconds(D3D12_STAGING_FREE_DELAY))
				{
					delete list;
					it = m_CommandListsArray.erase(it);
					continue;
				}
			}
			else if (list->State == CommandListState::Submitted)
			{
				if (list->Fence.IsCompleted())
				{
					list->OnCompletion();
				}
			}

			++it;
		}
	}

	void D3D12CommandContext::WaitIdle()
	{
		m_Fence.WaitCompletion();
	}

	bool D3D12CommandContext::HasPendingBarriers(CommandContextType srcContext) const
	{
		if (m_SyncPoints.contains(srcContext))
		{
			const ContextSyncPoint& syncPoint = m_SyncPoints.at(srcContext);
			return !syncPoint.Barriers.empty();
		}

		return false;
	}

#if ATN_ENABLE_TRACY_GPU
	void D3D12CommandContext::TracyCollect()
	{
		if(m_TracyContext)
			TracyD3D12Collect(m_TracyContext);
	}

	void D3D12CommandContext::InitTracy()
	{
		bool copyQueueSupport = D3D12RHI::Get()->GetFeaturesInfo().Options3.CopyQueueTimestampQueriesSupported;
		String name = EnumUtils::CmdContextTypeToString(GetType());

		if (GetType() == CommandContextType::AsyncTransfer && !copyQueueSupport)
		{
			ATN_LOG_WARN(D3D12, "Failed to create tracy context for {}Context (copy queue does not support timestamps).", name);

			m_TracyContext = nullptr;
			return;
		}

		m_TracyContext = TracyD3D12Context(D3D12RHI::Get()->GetDevice(), m_Queue);
		TracyD3D12ContextName(m_TracyContext, name.c_str(), name.size());

		ATN_LOG_INFO(D3D12, "Initialized Tracy context for {}Context.", name);
	}
#endif
}
