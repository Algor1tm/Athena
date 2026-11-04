#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/CommandStream.h"
#include "Athena/Renderer/CommandContext.h"
#include "Athena/Renderer/RHI/RHI.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanCommandBuffer.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanStagingManager.h"

#include <vulkan/vulkan.h>

#if ATN_ENABLE_TRACY_GPU
	#include <tracy/TracyVulkan.hpp>
#endif

namespace Athena
{
	class VulkanCommandContext : public CommandContext
	{
	public:
		VulkanCommandContext(uint32 queueFamily, CommandContextType type);
		virtual ~VulkanCommandContext();

		virtual CommandContextType GetType() override { return m_Type; }

		virtual void Drain(CommandContextType srcContext, PipelineAccess waitDstStage) override;
		virtual bool Flush() override;
		virtual bool FlushAndWait() override;

		virtual void Transition(Texture* texture, const TextureRange& subresource, PipelineAccess src, PipelineAccess dst) override;
		virtual void Transition(GPUBuffer* buffer, PipelineAccess src, PipelineAccess dst) override;
		virtual void TransitionOwnership(Texture* texture, const TextureRange& subresource, PipelineAccess src, CommandContextType dstContext, PipelineAccess dst) override;
		virtual void TransitionOwnership(GPUBuffer* buffer, PipelineAccess src, CommandContextType dstContext, PipelineAccess dst) override;

		virtual void UploadBuffer(GPUBuffer* buffer, const Buffer& data, uint64 offset = 0) override;
		virtual void UploadTexture(Texture* texture, const Buffer& data) override;

		virtual void BeginDebugLabel(const String& name, const IntegerColor& color = IntegerColor::Black) override;
		virtual void EndDebugLabel() override;
		virtual void InsertDebugLabel(const String& name, const IntegerColor& color = IntegerColor::Black) override;
		virtual bool HasTracyContext() override;

	public:
		void BeginFrame();
		void EndFrame();

		VulkanCommandBuffer* GetCommandBuffer();
		uint32 GetTimelineNextValue() const { return m_TimelineNextValue; }
		VkSemaphore GetTimelineSemaphore() const { return m_TimelineSemaphore; }

		uint32 GetQueueFamily() const { return m_QueueFamily; }
		VkQueue GetQueue() const { return m_Queue; }
		VkCommandPool GetCommandPool() const { return m_Pool; }

#if ATN_ENABLE_TRACY_GPU
		tracy::VkCtx* GetTracyContext() const { return m_TracyContext; }
#endif

	private:
		void UpdateCommandBuffers();

		struct ContextSyncPoint
		{
			bool Pending = false;
			VkSemaphore WaitSemaphore = VK_NULL_HANDLE;
			uint32 TimelineWaitValue = 0;
			PipelineAccess WaitStage = PA_Unknown;
			std::vector<VkImageMemoryBarrier2> PendingImageAcquires;
			std::vector<VkBufferMemoryBarrier2> PendingBufferAcquires;
		};

		void AddPendingAcquireBarrier(const VkImageMemoryBarrier2& barrier, CommandContextType srcContext);
		void AddPendingAcquireBarrier(const VkBufferMemoryBarrier2& barrier, CommandContextType srcContext);
		bool HasPendingAcquireBarriers(CommandContextType srcContext);
		bool HasPendingSyncPoints();

		void InitTracy();

	private:
		CommandContextType m_Type;
		uint32 m_QueueFamily;
		VkQueue m_Queue;
		VkCommandPool m_Pool;
		VkSemaphore m_TimelineSemaphore;
		uint64 m_TimelineNextValue;

		std::vector<VulkanCommandBuffer*> m_CommandBuffersArray;
		VulkanCommandBuffer* m_CurrentCommandBuffer;

		std::array<ContextSyncPoint, (uint32)CommandContextType::NumContexts> m_SyncPoints;
		VulkanStagingManager m_StagingManager;

#if ATN_ENABLE_TRACY_GPU
		tracy::VkCtx* m_TracyContext;
		bool m_TracyContextHostCalibrated;
#endif
	};
}
