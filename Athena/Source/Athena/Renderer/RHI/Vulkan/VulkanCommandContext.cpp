#include "VulkanCommandContext.h"

#include "Athena/Renderer/RHI/Vulkan/VulkanRHI.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanUtils.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanBuffers.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanTexture.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanUtils.h"


namespace Athena
{
	enum class VulkanBarrierType
	{
		Access,
		QueueFamilyRelease,
		QueueFamilyAcquire
	};

	namespace Vulkan
	{
		static VkImageLayout GetOptimalImageLayout(PipelineAccess access)
		{
			if (access == PA_Unknown)		  return VK_IMAGE_LAYOUT_UNDEFINED;
			if (access == PA_CopySrc)		  return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			if (access == PA_SRVVertexShader) return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			if (access == PA_SRVPixelShader)  return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			if (access == PA_SRVCompute)	  return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			if (access == PA_SRVGraphics)	  return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			if (access == PA_SRVMask)		  return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			if (access == PA_DSVRead)		  return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			if (access == PA_CopyDst)		  return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			if (access == PA_UAV)			  return VK_IMAGE_LAYOUT_GENERAL;
			if (access == PA_RTV)			  return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			if (access == PA_DSVWrite)		  return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			checkf(false);
			return VK_IMAGE_LAYOUT_UNDEFINED;
		}

		static VkAccessFlags2 GetImageAccessMask(PipelineAccess access)
		{
			VkAccessFlags2 result = VK_ACCESS_2_NONE;

			if (access & PA_CopySrc)		 result |= VK_ACCESS_2_TRANSFER_READ_BIT;
			if (access & PA_SRVVertexShader) result |= VK_ACCESS_2_SHADER_READ_BIT;
			if (access & PA_SRVPixelShader)	 result |= VK_ACCESS_2_SHADER_READ_BIT;
			if (access & PA_SRVCompute)		 result |= VK_ACCESS_2_SHADER_READ_BIT;
			if (access & PA_DSVRead)		 result |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			if (access & PA_CopyDst)		 result |= VK_ACCESS_2_TRANSFER_WRITE_BIT;
			if (access & PA_UAV)			 result |= VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
			if (access & PA_RTV)			 result |= VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
			if (access & PA_DSVWrite)		 result |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			return result;
		}

		static VkAccessFlags2 GetBufferAccessMask(PipelineAccess access, GPUBufferType bufferType)
		{
			VkAccessFlags2 result = VK_ACCESS_2_NONE;

			if (access & PA_CopySrc)		 result |= VK_ACCESS_2_TRANSFER_READ_BIT;
			if (access & PA_SRVVertexShader) result |= VK_ACCESS_2_SHADER_READ_BIT;
			if (access & PA_SRVPixelShader)  result |= VK_ACCESS_2_SHADER_READ_BIT;
			if (access & PA_SRVCompute)		 result |= VK_ACCESS_2_SHADER_READ_BIT;
			if (access & PA_CopyDst)		 result |= VK_ACCESS_2_TRANSFER_WRITE_BIT;
			if (access & PA_UAV)			 result |= VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;

			if (access & PA_VertexInput)
			{
				if(bufferType == GPUBufferType::VertexBuffer)
					result |= VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
				else if (bufferType == GPUBufferType::IndexBuffer)
					result |= VK_ACCESS_2_INDEX_READ_BIT;
			}

			return result;
		}

		static VkPipelineStageFlags2 GetPipelineStage(PipelineAccess access)
		{
			VkPipelineStageFlags2 result = VK_PIPELINE_STAGE_2_NONE;

			if (access == PipelineAccess::PA_Unknown)
				return VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;

			if (access & PA_CopySrc)		 result |= VK_PIPELINE_STAGE_2_TRANSFER_BIT;
			if (access & PA_VertexInput)	 result |= VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
			if (access & PA_SRVVertexShader) result |= VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
			if (access & PA_SRVPixelShader)	 result |= VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
			if (access & PA_SRVCompute)		 result |= VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
			if (access & PA_DSVRead)		 result |= VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
			if (access & PA_CopyDst)		 result |= VK_PIPELINE_STAGE_2_TRANSFER_BIT;
			if (access & PA_UAV)			 result |= VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
			if (access & PA_RTV)			 result |= VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
			if (access & PA_DSVWrite)		 result |= VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;

			return result;
		}

		static VkImageSubresourceRange GetVulkanSubresourceRange(Texture* texture, const TextureRange& range)
		{
			VkImageSubresourceRange subresourceRange = {};
			subresourceRange.aspectMask = Vulkan::TranslateTextureAspect(range.Aspect, texture->GetFormat());
			subresourceRange.baseMipLevel = range.BaseMipLevel;
			subresourceRange.levelCount = range.MipLevelCount;
			subresourceRange.baseArrayLayer = range.BaseLayer;
			subresourceRange.layerCount = range.LayerCount * Vulkan::GetLayerMultiplier(texture->GetType());

			return subresourceRange;
		}

		static VkImageMemoryBarrier2 GetImageBarrier(Texture* texture, const TextureRange& range,
			PipelineAccess src, PipelineAccess dst, VulkanBarrierType type = VulkanBarrierType::Access, uint32 srcQueueFamily = VK_QUEUE_FAMILY_IGNORED, uint32 dstQueueFamily = VK_QUEUE_FAMILY_IGNORED)
		{
			VulkanTexture* vulkanTexture = ((VulkanTexture*)texture);

			VkImageMemoryBarrier2 barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
			barrier.srcQueueFamilyIndex = srcQueueFamily;
			barrier.dstQueueFamilyIndex = dstQueueFamily;
			barrier.image = ((VulkanTexture*)texture)->GetImage();
			barrier.subresourceRange.aspectMask = Vulkan::TranslateTextureAspect(range.Aspect, texture->GetFormat());
			barrier.subresourceRange.baseMipLevel = range.BaseMipLevel;
			barrier.subresourceRange.levelCount = range.MipLevelCount;
			barrier.subresourceRange.baseArrayLayer = range.BaseLayer;
			barrier.subresourceRange.layerCount = range.LayerCount * Vulkan::GetLayerMultiplier(texture->GetType());

			ensuref(vulkanTexture->AreSubresourcesSameLayout(barrier.subresourceRange));

			barrier.oldLayout = vulkanTexture->GetSubresLayout(range.BaseMipLevel, range.BaseLayer);
			barrier.newLayout = Vulkan::GetOptimalImageLayout(dst);

			barrier.srcAccessMask = Vulkan::GetImageAccessMask(src);
			barrier.srcStageMask = Vulkan::GetPipelineStage(src);
			barrier.dstAccessMask = Vulkan::GetImageAccessMask(dst);
			barrier.dstStageMask = Vulkan::GetPipelineStage(dst);

			if (type == VulkanBarrierType::QueueFamilyRelease)
			{
				barrier.dstAccessMask = 0;
				barrier.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
			}
			else if (type == VulkanBarrierType::QueueFamilyAcquire)
			{
				barrier.srcAccessMask = 0;
				barrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
			}

			return barrier;
		}

		static VkBufferMemoryBarrier2 GetBufferBarrier(GPUBuffer* buffer, uint64 size, uint64 offset,
			PipelineAccess src, PipelineAccess dst, VulkanBarrierType type = VulkanBarrierType::Access, uint32 srcQueueFamily = VK_QUEUE_FAMILY_IGNORED, uint32 dstQueueFamily = VK_QUEUE_FAMILY_IGNORED)
		{
			VkBufferMemoryBarrier2 barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
			barrier.srcQueueFamilyIndex = srcQueueFamily;
			barrier.dstQueueFamilyIndex = dstQueueFamily;
			barrier.buffer = ((VulkanGPUBuffer*)buffer)->GetVulkanBuffer();
			barrier.offset = offset;
			barrier.size = size;

			barrier.srcAccessMask = Vulkan::GetBufferAccessMask(src, buffer->GetType());
			barrier.srcStageMask = Vulkan::GetPipelineStage(src);
			barrier.dstAccessMask = Vulkan::GetBufferAccessMask(dst, buffer->GetType());
			barrier.dstStageMask = Vulkan::GetPipelineStage(dst);

			if (type == VulkanBarrierType::QueueFamilyRelease)
			{
				barrier.dstAccessMask = 0;
				barrier.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
			}
			else if (type == VulkanBarrierType::QueueFamilyAcquire)
			{
				barrier.srcAccessMask = 0;
				barrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
			}

			return barrier;
		}

		static void ExecuteBarriers(VulkanCommandBuffer* cmdBuffer, const VkImageMemoryBarrier2* imageBarriers, uint32 imageBarriersCount, 
			const VkBufferMemoryBarrier2* bufferBarriers, uint32 bufferBarrierCount)
		{
			VkDependencyInfo dependencyInfo = {};
			dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
			dependencyInfo.dependencyFlags = 0;
			dependencyInfo.memoryBarrierCount = 0;
			dependencyInfo.pMemoryBarriers = nullptr;
			dependencyInfo.imageMemoryBarrierCount = imageBarriersCount;
			dependencyInfo.pImageMemoryBarriers = imageBarriers;
			dependencyInfo.bufferMemoryBarrierCount = bufferBarrierCount;
			dependencyInfo.pBufferMemoryBarriers = bufferBarriers;

			vkCmdPipelineBarrier2(cmdBuffer->CmdBuffer, &dependencyInfo);
		}
	}


	VulkanCommandContext::VulkanCommandContext(uint32 queueFamily, CommandContextType type)
		: m_QueueFamily(queueFamily), 
		m_Type(type)
	{
		vkGetDeviceQueue(Vulkan::GetLogicalDevice(), m_QueueFamily, 0, &m_Queue);

		VkCommandPoolCreateInfo commandPoolCI = {};
		commandPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolCI.queueFamilyIndex = m_QueueFamily;
		commandPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		VK_CHECK(vkCreateCommandPool(Vulkan::GetLogicalDevice(), &commandPoolCI, nullptr, &m_Pool));
		VK_SET_OBJECT_NAME(m_Pool, VK_OBJECT_TYPE_COMMAND_POOL, std::format("{}_CommandPool", EnumUtils::CmdContextTypeToString(GetType())));

		m_CurrentCommandBuffer = nullptr;

		VkSemaphoreTypeCreateInfo typeInfo{};
		typeInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
		typeInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
		typeInfo.initialValue = 0;
		m_TimelineNextValue = 1;

		VkSemaphoreCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		createInfo.pNext = &typeInfo;

		VK_CHECK(vkCreateSemaphore(Vulkan::GetLogicalDevice(), &createInfo, nullptr, &m_TimelineSemaphore));
		VK_SET_OBJECT_NAME(m_TimelineSemaphore, VK_OBJECT_TYPE_SEMAPHORE, std::format("{}_TimelineSemaphore", EnumUtils::CmdContextTypeToString(GetType())));

#if ATN_ENABLE_TRACY_GPU
		InitTracy();
#endif
	}

	VulkanCommandContext::~VulkanCommandContext()
	{
#if ATN_ENABLE_TRACY_GPU
		if(m_TracyContext)
			TracyVkDestroy(m_TracyContext);
#endif

		for (uint32 i = 0; i < m_CommandBuffersArray.size(); ++i)
		{
			VulkanCommandBuffer* cmdBuffer = m_CommandBuffersArray[i];
			cmdBuffer->WaitCompletion();
			delete cmdBuffer;
		}

		uint64 signaledValue = 0;
		VK_CHECK(vkGetSemaphoreCounterValue(Vulkan::GetLogicalDevice(), m_TimelineSemaphore, &signaledValue));
		m_StagingManager.Update(signaledValue);
		m_StagingManager.Clear();

		vkDestroySemaphore(Vulkan::GetLogicalDevice(), m_TimelineSemaphore, nullptr);
		vkDestroyCommandPool(Vulkan::GetLogicalDevice(), m_Pool, nullptr);
	}

	void VulkanCommandContext::BeginFrame()
	{
		TRACY_PROFILE_FUNC();

		UpdateCommandBuffers();
	}

	void VulkanCommandContext::EndFrame()
	{
#if ATN_ENABLE_TRACY_GPU
		if (m_TracyContext && !m_TracyContextHostCalibrated)
		{
			VulkanCommandBuffer* cmdBuffer = GetCommandBuffer();
			TracyVkCollect(m_TracyContext, cmdBuffer->CmdBuffer);
		}
#endif

		Flush();

#if ATN_ENABLE_TRACY_GPU
		if (m_TracyContext && m_TracyContextHostCalibrated)
		{
			TracyVkCollectHost(m_TracyContext);
		}
#endif
	}

	void VulkanCommandContext::Drain(CommandContextType srcContextType, PipelineAccess waitDstStage)
	{
		TRACY_PROFILE_FUNC();

		Ref<VulkanCommandContext> srcContext = RHI::Get()->GetContext(srcContextType).As<VulkanCommandContext>();
		if (srcContext.Raw() == this)
		{
			return;
		}

		bool hasAnyCommands = srcContext->Flush();

		// Nothing to wait on
		if (!hasAnyCommands && !HasPendingAcquireBarriers(srcContextType))
			return;

		ContextSyncPoint& syncPoint = m_SyncPoints[(uint32)srcContextType];
		ensure(syncPoint.Pending == false, "Previous drain was not flushed!");

		syncPoint.Pending = true;
		syncPoint.WaitSemaphore = srcContext->GetTimelineSemaphore();
		syncPoint.TimelineWaitValue = srcContext->GetTimelineNextValue() - 1;
		syncPoint.WaitStage = waitDstStage;
	}

	bool VulkanCommandContext::Flush()
	{
		TRACY_PROFILE_FUNC();

		// If we have active command buffer submit him with wait semaphores
		if (m_CurrentCommandBuffer)
		{
			VulkanCommandBuffer* cmdBuffer = m_CurrentCommandBuffer;
			cmdBuffer->End();

			// Add wait semaphores
			for (uint32 i = 0; i < m_SyncPoints.size(); ++i)
			{
				if (GetType() == (CommandContextType)i)
					continue;

				const ContextSyncPoint& syncPoint = m_SyncPoints[i];

				if (!syncPoint.Pending)
					continue;

				cmdBuffer->AddDependency(syncPoint.WaitSemaphore, syncPoint.TimelineWaitValue, Vulkan::GetPipelineStage(syncPoint.WaitStage));
			}

			// Submit
			VkSemaphoreSubmitInfo semaphoreSignalInfo = {};
			semaphoreSignalInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
			semaphoreSignalInfo.semaphore = m_TimelineSemaphore;
			semaphoreSignalInfo.value = m_TimelineNextValue;
			semaphoreSignalInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
			semaphoreSignalInfo.deviceIndex = 0;

			cmdBuffer->Submit(m_Queue, &semaphoreSignalInfo);
			m_TimelineNextValue++;

			m_CurrentCommandBuffer = nullptr;
			UpdateCommandBuffers();
		}

		// Add resources acquire barriers
		for (uint32 i = 0; i < m_SyncPoints.size(); ++i)
		{
			CommandContextType contextType = (CommandContextType)i;
			ContextSyncPoint& syncPoint = m_SyncPoints[i];

			if (!syncPoint.Pending)
				continue;

			syncPoint.Pending = false;

			if (GetType() == contextType)
				continue;

			if (!HasPendingAcquireBarriers(contextType))
				continue;

			VulkanCommandBuffer* cmdBuffer = GetCommandBuffer();

			Vulkan::ExecuteBarriers(cmdBuffer, 
				syncPoint.PendingImageAcquires.data(), syncPoint.PendingImageAcquires.size(),
				syncPoint.PendingBufferAcquires.data(), syncPoint.PendingBufferAcquires.size());

			syncPoint.PendingImageAcquires.clear();
			syncPoint.PendingBufferAcquires.clear();
		}

		uint64 signaledValue = 0;
		VK_CHECK(vkGetSemaphoreCounterValue(Vulkan::GetLogicalDevice(), m_TimelineSemaphore, &signaledValue));
		m_StagingManager.Update(signaledValue);

		return true;
	}

	bool VulkanCommandContext::FlushAndWait()
	{
		TRACY_PROFILE_FUNC();

		if (!m_CurrentCommandBuffer)
			return false;

		VulkanCommandBuffer* cmdBuffer = GetCommandBuffer();
		Flush();
		cmdBuffer->WaitCompletion();

		return true;
	}

	void VulkanCommandContext::Transition(Texture* texture, const TextureRange& range, PipelineAccess src, PipelineAccess dst)
	{
		VkImageMemoryBarrier2 barrier = Vulkan::GetImageBarrier(texture, range, src, dst);
		Vulkan::ExecuteBarriers(GetCommandBuffer(), &barrier, 1, nullptr, 0);

		((VulkanTexture*)texture)->SetSubresLayout(barrier.newLayout, barrier.subresourceRange);
	}

	void VulkanCommandContext::Transition(GPUBuffer* buffer, PipelineAccess src, PipelineAccess dst)
	{
		VkBufferMemoryBarrier2 barrier = Vulkan::GetBufferBarrier(buffer, buffer->GetSize(), 0, src, dst);
		Vulkan::ExecuteBarriers(GetCommandBuffer(), nullptr, 0, &barrier, 1);
	}

	void VulkanCommandContext::TransitionOwnership(Texture* texture, const TextureRange& range, PipelineAccess src, CommandContextType dstContextType, PipelineAccess dst)
	{
		Ref<VulkanCommandContext> dstContext = RHI::Get()->GetContext(dstContextType).As<VulkanCommandContext>();
		if (dstContext.Raw() == this)
		{
			Transition(texture, range, src, dst);
			return;
		}
		if (dstContext->GetQueueFamily() == m_QueueFamily)
		{
			Transition(texture, range, src, dst);
			return;
		}

		VkImageMemoryBarrier2 releaseBarrier = Vulkan::GetImageBarrier(texture, range, src, dst, 
			VulkanBarrierType::QueueFamilyRelease, m_QueueFamily, dstContext->GetQueueFamily());

		Vulkan::ExecuteBarriers(GetCommandBuffer(), &releaseBarrier, 1, nullptr, 0);

		VkImageMemoryBarrier2 acquireBarrier = Vulkan::GetImageBarrier(texture, range, src, dst,
			VulkanBarrierType::QueueFamilyAcquire, m_QueueFamily, dstContext->GetQueueFamily());

		dstContext->AddPendingAcquireBarrier(acquireBarrier, GetType());

		((VulkanTexture*)texture)->SetSubresLayout(acquireBarrier.newLayout, acquireBarrier.subresourceRange);
	}

	void VulkanCommandContext::TransitionOwnership(GPUBuffer* buffer, PipelineAccess src, CommandContextType dstContextType, PipelineAccess dst)
	{
		Ref<VulkanCommandContext> dstContext = RHI::Get()->GetContext(dstContextType).As<VulkanCommandContext>();
		if (dstContext.Raw() == this)
		{
			Transition(buffer, src, dst);
			return;
		}
		if (dstContext->GetQueueFamily() == m_QueueFamily)
		{
			Transition(buffer, src, dst);
			return;
		}

		VkBufferMemoryBarrier2 releaseBarrier = Vulkan::GetBufferBarrier(buffer, buffer->GetSize(), 0, src, dst,
			VulkanBarrierType::QueueFamilyRelease, m_QueueFamily, dstContext->GetQueueFamily());

		Vulkan::ExecuteBarriers(GetCommandBuffer(), nullptr, 0, &releaseBarrier, 1);

		VkBufferMemoryBarrier2 acquireBarrier = Vulkan::GetBufferBarrier(buffer, buffer->GetSize(), 0, src, dst,
			VulkanBarrierType::QueueFamilyAcquire, m_QueueFamily, dstContext->GetQueueFamily());

		dstContext->AddPendingAcquireBarrier(acquireBarrier, GetType());
	}

	void VulkanCommandContext::UploadBuffer(GPUBuffer* buffer, const Buffer& data, uint64 offset)
	{
		VulkanCommandBuffer* cmdBuffer = GetCommandBuffer();

		check(buffer->GetSize() >= data.Size() + offset && data.Data() != nullptr, "Invalid initialization buffer for VertexBuffer!");

		VulkanBufferAllocation stagingBuffer = m_StagingManager.GetBuffer(buffer->GetSize());

		void* mappedData = stagingBuffer.MapMemory();
		memcpy(mappedData, data.Data(), data.Size());
		stagingBuffer.UnmapMemory();

		VkBuffer vulkanBuffer = ((VulkanGPUBuffer*)buffer)->GetVulkanBuffer();

		VkBufferCopy copyRegion = {};
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = offset;
		copyRegion.size = data.Size();

		vkCmdCopyBuffer(cmdBuffer->CmdBuffer, stagingBuffer.GetBuffer(), vulkanBuffer, 1, &copyRegion);

		m_StagingManager.MarkAsUsed(stagingBuffer, m_TimelineNextValue);
	}

	void VulkanCommandContext::UploadTexture(Texture* texture, const Buffer& data)
	{
		VulkanCommandBuffer* cmdBuffer = GetCommandBuffer();

		VkDeviceSize imageSize = texture->GetWidth() * texture->GetHeight() * GetFormatProperties(texture->GetFormat()).BytesPerPixel;

		ensure(data.Size() <= imageSize, "Texture upload buffer is too small!");

		VkDeviceSize bufferSize = texture->GetInfo().Layers * Vulkan::GetLayerMultiplier(texture->GetType()) * imageSize;
		VulkanBufferAllocation stagingBuffer = m_StagingManager.GetBuffer(bufferSize);

		void* mappedMemory = stagingBuffer.MapMemory();

		for (uint32 offset = 0; offset < texture->GetInfo().Layers * imageSize; offset += imageSize)
			memcpy((byte*)mappedMemory + offset, data.Data(), imageSize);

		stagingBuffer.UnmapMemory();

		VkImage image = ((VulkanTexture*)texture)->GetImage();

		VkBufferImageCopy region = {};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = Vulkan::TranslateTextureAspect(TextureAspect::AUTO, texture->GetFormat());
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = texture->GetInfo().Layers * Vulkan::GetLayerMultiplier(texture->GetType());

		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { texture->GetWidth(), texture->GetHeight(), 1 };

		vkCmdCopyBufferToImage(cmdBuffer->CmdBuffer, stagingBuffer.GetBuffer(), image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		m_StagingManager.MarkAsUsed(stagingBuffer, m_TimelineNextValue);
	}

	void VulkanCommandContext::BeginDebugLabel(const String& name, const IntegerColor& color)
	{
		VK_BEGIN_DEBUG_LABEL(GetCommandBuffer()->CmdBuffer, name, color);
	}

	void VulkanCommandContext::EndDebugLabel()
	{
		VK_END_DEBUG_LABEL(GetCommandBuffer()->CmdBuffer);
	}

	void VulkanCommandContext::InsertDebugLabel(const String& name, const IntegerColor& color)
	{
		VK_INSERT_DEBUG_LABEL(GetCommandBuffer()->CmdBuffer, name, color);
	}

	bool VulkanCommandContext::HasTracyContext()
	{
		return m_TracyContext != nullptr;
	}

	VulkanCommandBuffer* VulkanCommandContext::GetCommandBuffer()
	{
		if (m_CurrentCommandBuffer)
			return m_CurrentCommandBuffer;

		for (uint32 i = 0; i < m_CommandBuffersArray.size(); ++i)
		{
			VulkanCommandBuffer* buffer = m_CommandBuffersArray[i];
			if (buffer->State == CommandBufferState::Idle)
			{
				m_CurrentCommandBuffer = buffer;
				break;
			}
		}

		if (!m_CurrentCommandBuffer)
		{
			m_CurrentCommandBuffer = m_CommandBuffersArray.emplace_back(new VulkanCommandBuffer(this));
		}

		m_CurrentCommandBuffer->Begin();
		VK_SET_OBJECT_NAME(m_CurrentCommandBuffer->CmdBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER,
			std::format("{}_CommandBuffer_{}", EnumUtils::CmdContextTypeToString(GetType()), m_TimelineNextValue - 1));

		VK_SET_OBJECT_NAME(m_CurrentCommandBuffer->SignalFence, VK_OBJECT_TYPE_FENCE,
			std::format("{}_Fence_{}", EnumUtils::CmdContextTypeToString(GetType()), m_TimelineNextValue - 1));

		return m_CurrentCommandBuffer;
	}

	void VulkanCommandContext::UpdateCommandBuffers()
	{
		TRACY_PROFILE_FUNC();

		std::vector<uint32> buffersToDelete;

		for (auto it = m_CommandBuffersArray.begin(); it != m_CommandBuffersArray.end();)
		{
			VulkanCommandBuffer* buffer = *it;

			if (buffer->State == CommandBufferState::Idle)
			{
				if (Time::GetSystemCurrentTime() - buffer->LastUsedTime >= Time::Seconds(VULKAN_STAGING_FREE_DELAY))
				{
					delete buffer;
					it = m_CommandBuffersArray.erase(it);
					continue;
				}
			}
			else if (buffer->State == CommandBufferState::Submitted)
			{
				VkResult result = vkGetFenceStatus(Vulkan::GetLogicalDevice(), buffer->SignalFence);

				if (result == VK_SUCCESS)
				{
					buffer->OnCompletion();
				}
			}

			++it;
		}
	}

	void VulkanCommandContext::AddPendingAcquireBarrier(const VkImageMemoryBarrier2& barrier, CommandContextType srcContext)
	{
		m_SyncPoints[(uint32)srcContext].PendingImageAcquires.push_back(barrier);
	}

	void VulkanCommandContext::AddPendingAcquireBarrier(const VkBufferMemoryBarrier2& barrier, CommandContextType srcContext)
	{
		m_SyncPoints[(uint32)srcContext].PendingBufferAcquires.push_back(barrier);
	}

	bool VulkanCommandContext::HasPendingAcquireBarriers(CommandContextType srcContext)
	{
		const ContextSyncPoint& syncPoint = m_SyncPoints[(uint32)srcContext];
		return !syncPoint.PendingImageAcquires.empty() || !syncPoint.PendingBufferAcquires.empty();
	}

	bool VulkanCommandContext::HasPendingSyncPoints()
	{
		for (uint32 i = 0; i < m_SyncPoints.size(); ++i)
		{
			if (GetType() == (CommandContextType)i)
				continue;

			const ContextSyncPoint& syncPoint = m_SyncPoints[i];

			if (syncPoint.Pending)
				return true;
		}

		return false;
	}

	void VulkanCommandContext::InitTracy()
	{
		m_TracyContext = nullptr;
		m_TracyContextHostCalibrated = false;

		VulkanDevice* device = VulkanRHI::Get()->GetDevice();

		const DeviceExtensionsInfo& extInfo = device->GetExtensionsInfo();
		const DeviceFeaturesInfo& featuresInfo = device->GetFeaturesInfo();
		const VkQueueFamilyProperties& queueFamily = device->GetQueueFamilyProps(m_QueueFamily);

		if (queueFamily.timestampValidBits == 0)
		{
			ATN_LOG_WARN(Vulkan, "Failed to create tracy context for {}Context (queue family does not support timestamps).", EnumUtils::CmdContextTypeToString(GetType()));
			return;
		}
		
		if (extInfo.CalibratedTimestampsEXTSupported && extInfo.HostQueryEXTSupported && featuresInfo.DeviceHostQueryResetFeatures.hostQueryReset)
		{
			uint32 domainCount = 0;
			extInfo.vkGetPhysicalDeviceCalibrateableTimeDomainsEXT(device->GetPhysicalDevice(), &domainCount, nullptr);

			std::vector<VkTimeDomainEXT> timeDomains(domainCount);
			extInfo.vkGetPhysicalDeviceCalibrateableTimeDomainsEXT(device->GetPhysicalDevice(), &domainCount, timeDomains.data());

			bool hasDeviceDomain = false;
			bool hasHostDomain = false;

			for (VkTimeDomainEXT domain : timeDomains)
			{
				hasDeviceDomain |= domain == VK_TIME_DOMAIN_DEVICE_EXT;

#if defined(ATN_PLATFORM_WINDOWS)
				hasHostDomain |= domain == VK_TIME_DOMAIN_QUERY_PERFORMANCE_COUNTER_EXT;
#else
				hasHostDomain |= domain == VK_TIME_DOMAIN_CLOCK_MONOTONIC_RAW_EXT;
#endif
			}

			if (hasDeviceDomain && hasHostDomain)
			{
				m_TracyContext = TracyVkContextHostCalibrated(device->GetPhysicalDevice(), device->GetLogicalDevice(), extInfo.vkResetQueryPoolEXT,
					extInfo.vkGetPhysicalDeviceCalibrateableTimeDomainsEXT, extInfo.vkGetCalibratedTimestampsEXT);

				m_TracyContextHostCalibrated = true;

				ATN_LOG_INFO(Vulkan, "Initialized Tracy host calibrated context for {}Context.", EnumUtils::CmdContextTypeToString(GetType()));
				return;
			}
		}

		bool isPureTransferContext = !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && !(queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT);
		if (isPureTransferContext)
		{
			ATN_LOG_WARN(Vulkan, "Failed to create tracy context for {}Context (transfer context supports only host calibrated tracy context).",
				EnumUtils::CmdContextTypeToString(GetType()));
			return;
		}

		VkCommandBufferAllocateInfo cmdBufAllocInfo = {};
		cmdBufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufAllocInfo.commandPool = m_Pool;
		cmdBufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdBufAllocInfo.commandBufferCount = 1;

		VkCommandBuffer tracyInitCmdBuffer;
		VK_CHECK(vkAllocateCommandBuffers(Vulkan::GetLogicalDevice(), &cmdBufAllocInfo, &tracyInitCmdBuffer));

		if (extInfo.CalibratedTimestampsEXTSupported)
		{
			m_TracyContext = TracyVkContextCalibrated(device->GetPhysicalDevice(), device->GetLogicalDevice(), m_Queue, tracyInitCmdBuffer,
				extInfo.vkGetPhysicalDeviceCalibrateableTimeDomainsEXT, extInfo.vkGetCalibratedTimestampsEXT);

			ATN_LOG_INFO(Vulkan, "Initialized Tracy calibrated context for {}Context.", EnumUtils::CmdContextTypeToString(GetType()));
		}
		else
		{
			m_TracyContext = TracyVkContext(device->GetPhysicalDevice(), device->GetLogicalDevice(), m_Queue, tracyInitCmdBuffer);

			ATN_LOG_INFO(Vulkan, "Initialized Tracy context for {}Context.", EnumUtils::CmdContextTypeToString(GetType()));
		}

		VK_CHECK(vkResetCommandBuffer(tracyInitCmdBuffer, 0));
	}
}
