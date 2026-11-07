#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/CommandStream.h"
#include "Athena/Renderer/CommandContext.h"
#include "Athena/Renderer/RHI/RHI.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanCommandBuffer.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanStagingPool.h"

#include <vulkan/vulkan.h>

#if ATN_ENABLE_TRACY_GPU
	#include <tracy/TracyVulkan.hpp>
	#include <stack>
#endif

namespace Athena
{
	class VulkanCommandContext : public CommandContext
	{
	public:
		VulkanCommandContext(uint32 queueFamily, CommandContextType type);
		virtual ~VulkanCommandContext();

		virtual CommandContextType GetType() override { return m_Type; }

		virtual void Drain(CommandContextType srcContext, ResourceState waitDstStage) override;
		virtual void Flush() override;
		virtual void FlushAndWait() override;

		virtual void Transition(Texture* texture, const TextureSubresourceRange& range, ResourceState dst) override;
		virtual void Transition(GPUBuffer* buffer, ResourceState dst) override;
		virtual void TransitionOwnership(Texture* texture, const TextureSubresourceRange& range, CommandContextType dstContext, ResourceState dst) override;
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

		VulkanCommandBuffer* GetCommandBuffer();

		uint32 GetTimelineLastCompletedValue() const;
		uint32 GetTimelineLastSignaledValue() const { return m_TimelineValue; }
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
			VkSemaphore WaitSemaphore = VK_NULL_HANDLE;
			uint32 TimelineWaitValue = 0;
			VkPipelineStageFlags WaitStage = 0;
			std::vector<VkImageMemoryBarrier2> PendingImageAcquires;
			std::vector<VkBufferMemoryBarrier2> PendingBufferAcquires;
		};

		bool HasPendingAcquireBarriers(CommandContextType srcContext);

#if ATN_ENABLE_TRACY_GPU
		void InitTracy();
#endif

	private:
		CommandContextType m_Type;
		uint32 m_QueueFamily;
		VkQueue m_Queue;
		VkCommandPool m_Pool;
		VkSemaphore m_TimelineSemaphore;
		uint64 m_TimelineValue;

		std::vector<VulkanCommandBuffer*> m_CommandBuffersArray;
		VulkanCommandBuffer* m_CurrentCommandBuffer;

		std::unordered_map<CommandContextType, ContextSyncPoint> m_SyncPoints;
		VulkanStagingPool m_StagingPool; // Store staging pool per queue to do not care about queue family ownership

#if ATN_ENABLE_TRACY_GPU
		tracy::VkCtx* m_TracyContext;
		bool m_TracyContextHostCalibrated;
		std::stack<tracy::VkCtxScope*> m_TracyZonesStack;
#endif
	};
}
