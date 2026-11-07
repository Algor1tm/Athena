#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/CommandStream.h"
#include "Athena/Renderer/CommandContext.h"
#include "Athena/Renderer/RHI/RHI.h"
#include "Athena/Renderer/RHI/D3D12/D3D12CommandList.h"

#if ATN_ENABLE_TRACY_GPU
	#include <tracy/TracyD3D12.hpp>
	#include <stack>
#endif


namespace Athena
{
	class D3D12CommandContext : public CommandContext
	{
	public:
		D3D12CommandContext(CommandContextType type);
		virtual ~D3D12CommandContext();

		virtual CommandContextType GetType() override { return m_Type; }

		virtual void Drain(CommandContextType srcContext, ResourceState waitDstStage) override;
		virtual void Flush() override;
		virtual void FlushAndWait() override;

		virtual void Transition(Texture* texture, const TextureSubresourceRange& range, ResourceState dst) override;
		virtual void Transition(GPUBuffer* buffer, ResourceState dst) override;
		virtual void TransitionOwnership(Texture* texture, const TextureSubresourceRange& subresource, CommandContextType dstContext, ResourceState dst) override;
		virtual void TransitionOwnership(GPUBuffer* buffer, CommandContextType dstContext, ResourceState dst) override;

		virtual void UploadBuffer(GPUBuffer* buffer, const Buffer& data, uint64 offset = 0) override;
		virtual void UploadTexture(Texture* texture, const Buffer& data) override;

#if ATN_ENABLE_GPU_DEBUG
		virtual void BeginMarker(const String& name) override;
		virtual void EndMarker() override;
		virtual void BeginQueueMarker(const String& name) override;
		virtual void EndQueueMarker() override;
		virtual void InsertMarker(const String& name) override;
#endif

#if ATN_ENABLE_TRACY_GPU
		virtual void BeginTracyZone(uint32 line, std::string_view fileName, std::string_view functionName, const String& name) override;
		virtual void EndTracyZone() override;
#endif

	public:
		void BeginFrame();
		void EndFrame();

		D3D12CommandList* GetCommandList();
		ID3D12CommandQueue* GetQueue() { return m_Queue; }

#if ATN_ENABLE_TRACY_GPU
		void TracyCollect();
#endif
		void WaitIdle();

	private:
		void UpdateCommandLists();
		bool HasPendingBarriers(CommandContextType srcContext) const;

		struct ContextSyncPoint
		{
			ID3D12Fence* Fence = nullptr;
			uint32 WaitValue = 0;
			std::vector<D3D12_RESOURCE_BARRIER> Barriers;
		};

#if ATN_ENABLE_TRACY_GPU
		void InitTracy();
#endif

	private:
		CommandContextType m_Type;
		ID3D12CommandQueue* m_Queue;
		D3D12Fence m_Fence;

		std::vector<D3D12CommandList*> m_CommandListsArray;
		D3D12CommandList* m_CurrentCommandList;

		std::unordered_map<CommandContextType, ContextSyncPoint> m_SyncPoints;

#if ATN_ENABLE_TRACY_GPU
		tracy::D3D12QueueCtx* m_TracyContext;
		std::stack<tracy::D3D12ZoneScope*> m_TracyZonesStack;
#endif
	};
}
