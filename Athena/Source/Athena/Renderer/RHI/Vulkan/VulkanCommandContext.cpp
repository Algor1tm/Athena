#include "VulkanCommandContext.h"

#include "Athena/Math/Random.h"
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
		static VkImageLayout GetOptimalImageLayout(ResourceState state)
		{
			switch (state)
			{
			case ResourceState::Init:				return VK_IMAGE_LAYOUT_UNDEFINED;
			case ResourceState::CopySrc:			return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			case ResourceState::SRVNonPixelShader:	return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			case ResourceState::SRVPixelShader:		return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			case ResourceState::SRVCompute:			return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			case ResourceState::SRVGraphics:		return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			case ResourceState::SRVAll:				return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			case ResourceState::DSVRead:			return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			case ResourceState::CopyDst:			return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			case ResourceState::UAV:				return VK_IMAGE_LAYOUT_GENERAL;
			case ResourceState::RTV:				return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			case ResourceState::DSVReadWrite:		return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			}

			checkf(false);
			return VK_IMAGE_LAYOUT_UNDEFINED;
		}

		static VkAccessFlags2 GetImageAccessMask(ResourceState state)
		{
			switch (state)
			{
			case ResourceState::Init:				return VK_ACCESS_2_NONE;
			case ResourceState::CopySrc:			return VK_ACCESS_2_TRANSFER_READ_BIT;
			case ResourceState::SRVNonPixelShader:	return VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
			case ResourceState::SRVPixelShader:		return VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
			case ResourceState::SRVCompute:			return VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
			case ResourceState::SRVGraphics:		return VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
			case ResourceState::SRVAll:				return VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
			case ResourceState::DSVRead:			return VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			case ResourceState::CopyDst:			return VK_ACCESS_2_TRANSFER_WRITE_BIT;
			case ResourceState::UAV:				return VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
			case ResourceState::RTV:				return VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
			case ResourceState::DSVReadWrite:		return VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			}

			checkf(false);
			return VK_ACCESS_2_NONE;
		}

		static VkAccessFlags2 GetBufferAccessMask(ResourceState state, GPUBufferType bufferType)
		{
			switch (state)
			{
			case ResourceState::Init:				return VK_ACCESS_2_NONE;
			case ResourceState::CopySrc:			return VK_ACCESS_2_TRANSFER_READ_BIT;
			case ResourceState::SRVNonPixelShader:	return VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
			case ResourceState::SRVPixelShader:		return VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
			case ResourceState::SRVCompute:			return VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
			case ResourceState::SRVGraphics:		return VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
			case ResourceState::SRVAll:				return VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
			case ResourceState::CopyDst:			return VK_ACCESS_2_TRANSFER_WRITE_BIT;
			case ResourceState::UAV:				return VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
			}

			if (state == ResourceState::VertexInput)
			{
				switch (bufferType)
				{
				case GPUBufferType::VertexBuffer: return VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
				case GPUBufferType::IndexBuffer:  return VK_ACCESS_2_INDEX_READ_BIT;
				}
			}

			checkf(false);
			return VK_ACCESS_2_NONE;
		}

		static VkPipelineStageFlags2 GetPipelineStage(ResourceState state)
		{
			switch (state)
			{
			case ResourceState::Init:				return VK_PIPELINE_STAGE_2_NONE;
			case ResourceState::VertexInput:		return VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
			case ResourceState::CopySrc:			return VK_PIPELINE_STAGE_2_TRANSFER_BIT;
			case ResourceState::SRVNonPixelShader:	return VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT;
			case ResourceState::SRVPixelShader:		return VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
			case ResourceState::SRVCompute:			return VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;

			case ResourceState::SRVGraphics:		return VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | 
				VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;

			case ResourceState::SRVAll:				return VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | 
				VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;

			case ResourceState::DSVRead:			return VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
			case ResourceState::CopyDst:			return VK_PIPELINE_STAGE_2_TRANSFER_BIT;
			case ResourceState::UAV:				return VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
			case ResourceState::RTV:				return VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
			case ResourceState::DSVReadWrite:		return VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
			}

			checkf(false);
			return VK_PIPELINE_STAGE_2_NONE;
		}

		static VkImageMemoryBarrier2 GetImageBarrier(Texture* texture, const TextureSubresourceRange& range,
			ResourceState src, ResourceState dst, VulkanBarrierType type = VulkanBarrierType::Access, uint32 srcQueueFamily = VK_QUEUE_FAMILY_IGNORED, uint32 dstQueueFamily = VK_QUEUE_FAMILY_IGNORED)
		{
			VulkanTexture* vulkanTexture = ((VulkanTexture*)texture);

			VkImageMemoryBarrier2 barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
			barrier.srcQueueFamilyIndex = srcQueueFamily;
			barrier.dstQueueFamilyIndex = dstQueueFamily;
			barrier.image = ((VulkanTexture*)texture)->GetImage();
			barrier.subresourceRange.aspectMask = Vulkan::TranslateTextureAspect(range.Plane, texture->GetFormat());
			barrier.subresourceRange.baseMipLevel = range.BaseMipLevel;
			barrier.subresourceRange.levelCount = range.MipLevelCount;
			barrier.subresourceRange.baseArrayLayer = range.BaseLayer;
			barrier.subresourceRange.layerCount = range.LayerCount * Vulkan::GetLayerMultiplier(texture->GetType());

			barrier.oldLayout = Vulkan::GetOptimalImageLayout(src);
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
			ResourceState src, ResourceState dst, VulkanBarrierType type = VulkanBarrierType::Access, uint32 srcQueueFamily = VK_QUEUE_FAMILY_IGNORED, uint32 dstQueueFamily = VK_QUEUE_FAMILY_IGNORED)
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

		static VkDebugUtilsLabelEXT GetDebugUtilsLabel(const String& name)
		{
			static uint64 counter = 0;
			float hue = Math::Fract(counter++ * Math::GoldenRatio<float>());

			HSVColor hsvColor = HSVColor(hue * 360.f, 65, 100.f);
			LinearColor color = hsvColor;

			VkDebugUtilsLabelEXT labelInfo = {};
			labelInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
			labelInfo.pLabelName = name.c_str();
			labelInfo.color[0] = color.r;
			labelInfo.color[1] = color.g;
			labelInfo.color[2] = color.b;
			labelInfo.color[3] = color.a;

			return labelInfo;
		}
	}


	VulkanCommandContext::VulkanCommandContext(uint32 queueFamily, CommandContextType type)
		: m_QueueFamily(queueFamily), 
		m_Type(type)
	{
		vkGetDeviceQueue(Vulkan::GetLogicalDevice(), m_QueueFamily, 0, &m_Queue);
		VK_SET_DEBUG_NAME(m_Queue, VK_OBJECT_TYPE_QUEUE, fmt::format("{}_Queue", EnumUtils::CmdContextTypeToString(GetType())));

		VkCommandPoolCreateInfo commandPoolCI = {};
		commandPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolCI.queueFamilyIndex = m_QueueFamily;
		commandPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		VK_CHECK(vkCreateCommandPool(Vulkan::GetLogicalDevice(), &commandPoolCI, nullptr, &m_Pool));
		VK_SET_DEBUG_NAME(m_Pool, VK_OBJECT_TYPE_COMMAND_POOL, fmt::format("{}_CommandPool", EnumUtils::CmdContextTypeToString(GetType())));

		m_CurrentCommandBuffer = nullptr;

		m_TimelineValue = 0;

		VkSemaphoreTypeCreateInfo typeInfo{};
		typeInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
		typeInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
		typeInfo.initialValue = m_TimelineValue;

		VkSemaphoreCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		createInfo.pNext = &typeInfo;

		VK_CHECK(vkCreateSemaphore(Vulkan::GetLogicalDevice(), &createInfo, nullptr, &m_TimelineSemaphore));
		VK_SET_DEBUG_NAME(m_TimelineSemaphore, VK_OBJECT_TYPE_SEMAPHORE, fmt::format("{}_TimelineSemaphore", EnumUtils::CmdContextTypeToString(GetType())));

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

		m_StagingPool.Update(GetTimelineLastCompletedValue());
		m_StagingPool.Clear();

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

	void VulkanCommandContext::Drain(CommandContextType srcContextType, ResourceState waitDstStage)
	{
		TRACY_PROFILE_FUNC();

		Ref<VulkanCommandContext> srcContext = RHI::Get()->GetContext(srcContextType).As<VulkanCommandContext>();
		if (srcContext.Raw() == this)
		{
			return;
		}

		srcContext->Flush();

		uint64 lastSignaledValue = srcContext->GetTimelineLastSignaledValue();
		bool anyPendingCommands = srcContext->GetTimelineLastCompletedValue() == lastSignaledValue;

		// Nothing to wait on
		if (!anyPendingCommands)
			return;

		ContextSyncPoint& syncPoint = m_SyncPoints[srcContextType];
		syncPoint.WaitSemaphore = srcContext->GetTimelineSemaphore();
		syncPoint.TimelineWaitValue = lastSignaledValue;
		syncPoint.WaitStage = Vulkan::GetPipelineStage(waitDstStage);
	}

	void VulkanCommandContext::Flush()
	{
		TRACY_PROFILE_FUNC();

		// If we have active command buffer submit him with wait semaphores
		if (m_CurrentCommandBuffer)
		{
			GPU_TRACY_END(this);

#if ATN_ENABLE_TRACY_GPU
			ensuref(m_TracyZonesStack.empty());
#endif

			VulkanCommandBuffer* cmdBuffer = m_CurrentCommandBuffer;
			cmdBuffer->End();

			for (const auto& [type, syncPoint] : m_SyncPoints)
			{
				if(syncPoint.WaitSemaphore != VK_NULL_HANDLE)
					cmdBuffer->AddDependency(syncPoint.WaitSemaphore, syncPoint.TimelineWaitValue, syncPoint.WaitStage);
			}

			m_TimelineValue++;
			cmdBuffer->AddSignal(m_TimelineSemaphore, m_TimelineValue, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT);
			cmdBuffer->Submit(m_Queue);

			m_CurrentCommandBuffer = nullptr;
			m_StagingPool.Update(GetTimelineLastCompletedValue());
		}

		// Add resources acquire barriers
		for (const auto& [type, syncPoint] : m_SyncPoints)
		{
			if (!HasPendingAcquireBarriers(type))
				continue;

			VulkanCommandBuffer* cmdBuffer = GetCommandBuffer();

			Vulkan::ExecuteBarriers(cmdBuffer,
				syncPoint.PendingImageAcquires.data(), syncPoint.PendingImageAcquires.size(),
				syncPoint.PendingBufferAcquires.data(), syncPoint.PendingBufferAcquires.size());
		}

		m_SyncPoints.clear();
	}

	void VulkanCommandContext::FlushAndWait()
	{
		TRACY_PROFILE_FUNC();

		if (!m_CurrentCommandBuffer)
			return;

		VulkanCommandBuffer* cmdBuffer = GetCommandBuffer();
		Flush();
		cmdBuffer->WaitCompletion();
	}

	void VulkanCommandContext::Transition(Texture* texture, const TextureSubresourceRange& range, ResourceState dst)
	{
		if (texture->AreSubresourcesSameState(range, dst))
			return;

		if (texture->AreSubresourcesSameState(range))
		{
			ResourceState src = texture->GetSubresState({ range.BaseMipLevel, range.BaseLayer, TexturePlane(range.GetFirstPlane()) });
			VkImageMemoryBarrier2 barrier = Vulkan::GetImageBarrier(texture, range, src, dst);
			Vulkan::ExecuteBarriers(GetCommandBuffer(), &barrier, 1, nullptr, 0);
		}
		else
		{
			uint32 firstPlane = range.GetFirstPlane();
			uint32 lastPlane = range.GetLastPlane(texture->GetPlaneCount());

			std::vector<VkImageMemoryBarrier2> barriers;
			barriers.reserve((lastPlane - firstPlane) * (range.LayerCount - range.BaseLayer) * (range.MipLevelCount - range.BaseMipLevel));

			for (uint32 plane = firstPlane; plane < lastPlane; ++plane)
			{
				for (uint32 layer = range.BaseLayer; layer < range.LayerCount; ++layer)
				{
					for (uint32 mip = range.BaseMipLevel; mip < range.MipLevelCount; ++mip)
					{
						TextureSubresource subresource = TextureSubresource(mip, layer, TexturePlane(plane));
						ResourceState src = texture->GetSubresState({ mip, layer, TexturePlane(plane) });

						VkImageMemoryBarrier2 barrier = Vulkan::GetImageBarrier(texture, subresource, src, dst);
						barriers.push_back(barrier);
					}
				}
			}

			Vulkan::ExecuteBarriers(GetCommandBuffer(), barriers.data(), barriers.size(), nullptr, 0);
		}

		texture->SetSubresState(dst, range);
	}

	void VulkanCommandContext::Transition(GPUBuffer* buffer, ResourceState dst)
	{
		if (buffer->GetState() == dst)
			return;

		ResourceState src = buffer->GetState();

		VkBufferMemoryBarrier2 barrier = Vulkan::GetBufferBarrier(buffer, buffer->GetSize(), 0, src, dst);
		Vulkan::ExecuteBarriers(GetCommandBuffer(), nullptr, 0, &barrier, 1);

		buffer->SetState(dst);
	}

	void VulkanCommandContext::TransitionOwnership(Texture* texture, const TextureSubresourceRange& range, CommandContextType dstContextType, ResourceState dst)
	{
		if (texture->AreSubresourcesSameState(range, dst))
			return;

		Ref<VulkanCommandContext> dstContext = RHI::Get()->GetContext(dstContextType).As<VulkanCommandContext>();

		if (dstContext.Raw() == this || dstContext->GetQueueFamily() == m_QueueFamily)
		{
			Transition(texture, range, dst);
			return;
		}

		if (texture->AreSubresourcesSameState(range))
		{
			ResourceState src = texture->GetSubresState({ range.BaseMipLevel, range.BaseLayer, TexturePlane(range.GetFirstPlane()) });

			VkImageMemoryBarrier2 releaseBarrier = Vulkan::GetImageBarrier(texture, range, src, dst,
				VulkanBarrierType::QueueFamilyRelease, m_QueueFamily, dstContext->GetQueueFamily());

			Vulkan::ExecuteBarriers(GetCommandBuffer(), &releaseBarrier, 1, nullptr, 0);

			VkImageMemoryBarrier2 acquireBarrier = Vulkan::GetImageBarrier(texture, range, src, dst,
				VulkanBarrierType::QueueFamilyAcquire, m_QueueFamily, dstContext->GetQueueFamily());

			dstContext->m_SyncPoints[GetType()].PendingImageAcquires.push_back(acquireBarrier);
		}
		else
		{
			uint32 firstPlane = range.GetFirstPlane();
			uint32 lastPlane = range.GetLastPlane(texture->GetPlaneCount());

			uint32 barriersCount = (lastPlane - firstPlane) * (range.LayerCount - range.BaseLayer) * (range.MipLevelCount - range.BaseMipLevel);

			std::vector<VkImageMemoryBarrier2> releaseBarriers;
			releaseBarriers.reserve(barriersCount);

			for (uint32 plane = firstPlane; plane < lastPlane; ++plane)
			{
				for (uint32 layer = range.BaseLayer; layer < range.LayerCount; ++layer)
				{
					for (uint32 mip = range.BaseMipLevel; mip < range.MipLevelCount; ++mip)
					{
						TextureSubresource subresource = TextureSubresource(mip, layer, TexturePlane(plane));
						ResourceState src = texture->GetSubresState({ mip, layer, TexturePlane(plane) });

						VkImageMemoryBarrier2 releaseBarrier = Vulkan::GetImageBarrier(texture, subresource, src, dst,
							VulkanBarrierType::QueueFamilyRelease, m_QueueFamily, dstContext->GetQueueFamily());

						releaseBarriers.push_back(releaseBarrier);

						VkImageMemoryBarrier2 acquireBarrier = Vulkan::GetImageBarrier(texture, subresource, src, dst,
							VulkanBarrierType::QueueFamilyAcquire, m_QueueFamily, dstContext->GetQueueFamily());

						dstContext->m_SyncPoints[GetType()].PendingImageAcquires.push_back(acquireBarrier);
					}
				}
			}

			Vulkan::ExecuteBarriers(GetCommandBuffer(), releaseBarriers.data(), releaseBarriers.size(), nullptr, 0);
		}

		texture->SetSubresState(dst, range);
	}

	void VulkanCommandContext::TransitionOwnership(GPUBuffer* buffer, CommandContextType dstContextType, ResourceState dst)
	{
		if (buffer->GetState() == dst)
			return;

		Ref<VulkanCommandContext> dstContext = RHI::Get()->GetContext(dstContextType).As<VulkanCommandContext>();

		if (dstContext.Raw() == this || dstContext->GetQueueFamily() == m_QueueFamily)
		{
			Transition(buffer, dst);
			return;
		}

		ResourceState src = buffer->GetState();

		VkBufferMemoryBarrier2 releaseBarrier = Vulkan::GetBufferBarrier(buffer, buffer->GetSize(), 0, src, dst,
			VulkanBarrierType::QueueFamilyRelease, m_QueueFamily, dstContext->GetQueueFamily());

		Vulkan::ExecuteBarriers(GetCommandBuffer(), nullptr, 0, &releaseBarrier, 1);

		VkBufferMemoryBarrier2 acquireBarrier = Vulkan::GetBufferBarrier(buffer, buffer->GetSize(), 0, src, dst,
			VulkanBarrierType::QueueFamilyAcquire, m_QueueFamily, dstContext->GetQueueFamily());

		dstContext->m_SyncPoints[GetType()].PendingBufferAcquires.push_back(acquireBarrier);

		buffer->SetState(dst);
	}

	void VulkanCommandContext::UploadBuffer(GPUBuffer* buffer, const Buffer& data, uint64 offset)
	{
		VulkanCommandBuffer* cmdBuffer = GetCommandBuffer();

		check(buffer->GetSize() >= data.Size() + offset && data.Data() != nullptr, "Invalid initialization buffer for VertexBuffer!");

		VulkanBufferAllocation stagingBuffer = m_StagingPool.GetBuffer(buffer->GetSize());

		void* mappedData = stagingBuffer.MapMemory();
		memcpy(mappedData, data.Data(), data.Size());
		stagingBuffer.UnmapMemory();

		VkBuffer vulkanBuffer = ((VulkanGPUBuffer*)buffer)->GetVulkanBuffer();

		VkBufferCopy copyRegion = {};
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = offset;
		copyRegion.size = data.Size();

		vkCmdCopyBuffer(cmdBuffer->CmdBuffer, stagingBuffer.GetBuffer(), vulkanBuffer, 1, &copyRegion);

		m_StagingPool.MarkAsUsed(stagingBuffer, m_TimelineValue + 1);
	}

	void VulkanCommandContext::UploadTexture(Texture* texture, const Buffer& data)
	{
		VulkanCommandBuffer* cmdBuffer = GetCommandBuffer();

		VkDeviceSize imageSize = texture->GetWidth() * texture->GetHeight() * GetFormatProperties(texture->GetFormat()).BytesPerPixel;

		ensure(data.Size() <= imageSize, "Texture upload buffer is too small!");

		VkDeviceSize bufferSize = texture->GetInfo().Layers * Vulkan::GetLayerMultiplier(texture->GetType()) * imageSize;
		VulkanBufferAllocation stagingBuffer = m_StagingPool.GetBuffer(bufferSize);

		void* mappedMemory = stagingBuffer.MapMemory();

		for (uint32 offset = 0; offset < texture->GetInfo().Layers * imageSize; offset += imageSize)
			memcpy((byte*)mappedMemory + offset, data.Data(), imageSize);

		stagingBuffer.UnmapMemory();

		VkImage image = ((VulkanTexture*)texture)->GetImage();

		VkBufferImageCopy region = {};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = Vulkan::TranslateTextureAspect(TexturePlane::ALL, texture->GetFormat());
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = texture->GetInfo().Layers * Vulkan::GetLayerMultiplier(texture->GetType());

		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { texture->GetWidth(), texture->GetHeight(), 1 };

		vkCmdCopyBufferToImage(cmdBuffer->CmdBuffer, stagingBuffer.GetBuffer(), image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		m_StagingPool.MarkAsUsed(stagingBuffer, m_TimelineValue + 1);
	}

#if ATN_ENABLE_GPU_DEBUG
	void VulkanCommandContext::BeginMarker(const String& name)
	{
		if (!VulkanRHI::Get()->GetExtensionsInfo().DebugUtilsEXTSupported)
			return;

		VkDebugUtilsLabelEXT label = Vulkan::GetDebugUtilsLabel(name);

		PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT = VulkanRHI::Get()->GetExtensionsInfo().vkCmdBeginDebugUtilsLabelEXT;
		vkCmdBeginDebugUtilsLabelEXT(GetCommandBuffer()->CmdBuffer, &label);
	}

	void VulkanCommandContext::EndMarker()
	{
		if (!VulkanRHI::Get()->GetExtensionsInfo().DebugUtilsEXTSupported)
			return;

		PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT = VulkanRHI::Get()->GetExtensionsInfo().vkCmdEndDebugUtilsLabelEXT;
		vkCmdEndDebugUtilsLabelEXT(GetCommandBuffer()->CmdBuffer);
	}

	void VulkanCommandContext::BeginQueueMarker(const String& name)
	{
		if (!VulkanRHI::Get()->GetExtensionsInfo().DebugUtilsEXTSupported)
			return;

		VkDebugUtilsLabelEXT label = Vulkan::GetDebugUtilsLabel(name);

		PFN_vkQueueBeginDebugUtilsLabelEXT vkQueueBeginDebugUtilsLabelEXT = VulkanRHI::Get()->GetExtensionsInfo().vkQueueBeginDebugUtilsLabelEXT;
		vkQueueBeginDebugUtilsLabelEXT(m_Queue, &label);
	}

	void VulkanCommandContext::EndQueueMarker()
	{
		if (!VulkanRHI::Get()->GetExtensionsInfo().DebugUtilsEXTSupported)
			return;

		PFN_vkQueueEndDebugUtilsLabelEXT vkQueueEndDebugUtilsLabelEXT = VulkanRHI::Get()->GetExtensionsInfo().vkQueueEndDebugUtilsLabelEXT;
		vkQueueEndDebugUtilsLabelEXT(m_Queue);
	}

	void VulkanCommandContext::InsertMarker(const String& name)
	{
		if (!VulkanRHI::Get()->GetExtensionsInfo().DebugUtilsEXTSupported)
			return;

		VkDebugUtilsLabelEXT label = Vulkan::GetDebugUtilsLabel(name);

		PFN_vkCmdInsertDebugUtilsLabelEXT vkCmdInsertDebugUtilsLabelEXT = VulkanRHI::Get()->GetExtensionsInfo().vkCmdInsertDebugUtilsLabelEXT;
		vkCmdInsertDebugUtilsLabelEXT(GetCommandBuffer()->CmdBuffer, &label);
	}
#endif

#if ATN_ENABLE_TRACY_GPU
	void VulkanCommandContext::BeginTracyZone(uint32 line, std::string_view fileName, std::string_view functionName, const String& name)
	{
		if (!m_TracyContext)
			return;

		m_TracyZonesStack.push(new tracy::VkCtxScope(
			m_TracyContext,
			line,
			fileName.data(), fileName.size(),
			functionName.data(), functionName.size(),
			name.data(), name.size(),
			GetCommandBuffer()->CmdBuffer,
			true));
	}

	void VulkanCommandContext::EndTracyZone()
	{
		if (!m_TracyContext)
			return;

		tracy::VkCtxScope* zone = m_TracyZonesStack.top();
		m_TracyZonesStack.pop();

		delete zone;
	}
#endif

	uint32 VulkanCommandContext::GetTimelineLastCompletedValue() const
	{
		uint64 signaledValue = 0;
		VK_CHECK(vkGetSemaphoreCounterValue(Vulkan::GetLogicalDevice(), m_TimelineSemaphore, &signaledValue));

		return signaledValue;
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
			else if (buffer->State == CommandBufferState::Submitted)
			{
				VkResult result = vkGetFenceStatus(Vulkan::GetLogicalDevice(), buffer->SignalFence);

				if (result == VK_SUCCESS)
				{
					buffer->OnCompletion();
					m_CurrentCommandBuffer = buffer;
					break;
				}
			}
		}

		if (!m_CurrentCommandBuffer)
		{
			m_CurrentCommandBuffer = m_CommandBuffersArray.emplace_back(new VulkanCommandBuffer(m_Pool));
		}

		m_CurrentCommandBuffer->Begin();
		VK_SET_DEBUG_NAME(m_CurrentCommandBuffer->CmdBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER,
			fmt::format("{}_CommandBuffer_{}", EnumUtils::CmdContextTypeToString(GetType()), m_TimelineValue));

		VK_SET_DEBUG_NAME(m_CurrentCommandBuffer->SignalFence, VK_OBJECT_TYPE_FENCE,
			fmt::format("{}_Fence_{}", EnumUtils::CmdContextTypeToString(GetType()), m_TimelineValue));

		GPU_TRACY_BEGIN(this, fmt::format("{}Commands", EnumUtils::CmdContextTypeToString(GetType())));

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

	bool VulkanCommandContext::HasPendingAcquireBarriers(CommandContextType srcContext)
	{
		if (m_SyncPoints.contains(srcContext))
		{
			const ContextSyncPoint& syncPoint = m_SyncPoints.at(srcContext);
			return !syncPoint.PendingImageAcquires.empty() || !syncPoint.PendingBufferAcquires.empty();
		}

		return false;
	}

#if ATN_ENABLE_TRACY_GPU
	void VulkanCommandContext::InitTracy()
	{
		m_TracyContext = nullptr;
		m_TracyContextHostCalibrated = false;

		VulkanDevice* device = VulkanRHI::Get()->GetDevice();

		const DeviceExtensionsInfo& extInfo = device->GetExtensionsInfo();
		const DeviceFeaturesInfo& featuresInfo = device->GetFeaturesInfo();
		const VkQueueFamilyProperties& queueFamily = device->GetQueueFamilyProps(m_QueueFamily);

		String name = EnumUtils::CmdContextTypeToString(GetType());

		if (queueFamily.timestampValidBits == 0)
		{
			ATN_LOG_WARN(Vulkan, "Failed to create tracy context for {}Context (queue family does not support timestamps).", name);
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

				TracyVkContextName(m_TracyContext, name.c_str(), name.size());
				m_TracyContextHostCalibrated = true;

				ATN_LOG_INFO(Vulkan, "Initialized Tracy host calibrated context for {}Context.", name);
				return;
			}
		}

		bool isPureTransferContext = !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && !(queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT);
		if (isPureTransferContext)
		{
			ATN_LOG_WARN(Vulkan, "Failed to create tracy context for {}Context (transfer context supports only host calibrated tracy context).", name);
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

			ATN_LOG_INFO(Vulkan, "Initialized Tracy calibrated context for {}Context.", name);
		}
		else
		{
			m_TracyContext = TracyVkContext(device->GetPhysicalDevice(), device->GetLogicalDevice(), m_Queue, tracyInitCmdBuffer);

			ATN_LOG_INFO(Vulkan, "Initialized Tracy context for {}Context.", name);
		}

		VK_CHECK(vkResetCommandBuffer(tracyInitCmdBuffer, 0));

		TracyVkContextName(m_TracyContext, name.c_str(), name.size());
	}
#endif
}
