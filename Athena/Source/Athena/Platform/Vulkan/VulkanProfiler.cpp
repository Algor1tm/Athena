#include "VulkanProfiler.h"

#include "Athena/Platform/Vulkan/VulkanUtils.h"


namespace Athena
{
	VulkanProfiler::VulkanProfiler()
	{
		Renderer::Submit([this]()
		{
			// Create time queury pool
			{
				VkQueryPoolCreateInfo queryPoolInfo = {};
				queryPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
				queryPoolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
				queryPoolInfo.queryCount = m_MaxTimestampsCount * Renderer::GetFramesInFlight();
				
				VK_CHECK(vkCreateQueryPool(VulkanContext::GetLogicalDevice(), &queryPoolInfo, VulkanContext::GetAllocator(), &m_TimeQueryPool));
				
				vkResetQueryPool(VulkanContext::GetLogicalDevice(), m_TimeQueryPool, 0, queryPoolInfo.queryCount);
				
				m_TimestampsCount.resize(Renderer::GetFramesInFlight());
				m_Timestamps.resize(Renderer::GetFramesInFlight());
				m_ResolvedTimeStats.resize(Renderer::GetFramesInFlight());

				// Time stored in ms
				m_Frequency = (uint64)(1000000ll / Renderer::GetRenderCaps().TimestampPeriod);
			}

			// Pipeline statistics query pool
			{
				VkQueryPoolCreateInfo queryPoolInfo = {};
				queryPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
				queryPoolInfo.queryType = VK_QUERY_TYPE_PIPELINE_STATISTICS;
				queryPoolInfo.queryCount = m_MaxPipelineQueriesCount * Renderer::GetFramesInFlight();
				queryPoolInfo.pipelineStatistics =
					VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT |
					VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT |
					VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT |
					VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT |
					VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT |
					VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT |
					VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT |
					VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT |
					VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT;

				VK_CHECK(vkCreateQueryPool(VulkanContext::GetLogicalDevice(), &queryPoolInfo, VulkanContext::GetAllocator(), &m_PipelineStatsQueryPool));

				vkResetQueryPool(VulkanContext::GetLogicalDevice(), m_PipelineStatsQueryPool, 0, queryPoolInfo.queryCount);

				m_PipelineQueriesCount.resize(Renderer::GetFramesInFlight());
				m_ResolvedPipelineStats.resize(Renderer::GetFramesInFlight());
			}
		});
	}

	VulkanProfiler::~VulkanProfiler()
	{
		Renderer::SubmitResourceFree([timeQueryPool = m_TimeQueryPool, pipelineQueryPool = m_PipelineStatsQueryPool]()
		{
			vkDestroyQueryPool(VulkanContext::GetLogicalDevice(), timeQueryPool, VulkanContext::GetAllocator());
			vkDestroyQueryPool(VulkanContext::GetLogicalDevice(), pipelineQueryPool, VulkanContext::GetAllocator());
		});
	}

	void VulkanProfiler::Reset()
	{
		Renderer::Submit([this]()
		{
			// Previous frame index
			m_QueryFrameIndex = (Renderer::GetCurrentFrameIndex() + (Renderer::GetFramesInFlight() - 1)) % Renderer::GetFramesInFlight();

			// Reset time query
			{
				uint32 start = m_MaxTimestampsCount * Renderer::GetCurrentFrameIndex();
				uint32 count = m_TimestampsCount[Renderer::GetCurrentFrameIndex()];

				if (count > 0)
				{
					auto& timestamps = m_Timestamps[Renderer::GetCurrentFrameIndex()];
					auto& resolvedTimeStats = m_ResolvedTimeStats[Renderer::GetCurrentFrameIndex()];

					vkGetQueryPoolResults(VulkanContext::GetLogicalDevice(),
						m_TimeQueryPool,
						start,
						count,
						count * sizeof(uint64),
						timestamps.data(),
						sizeof(uint64),
						VK_QUERY_RESULT_64_BIT);

					for (uint32 i = 0; i < count; i += 2)
					{
						double resolvedTime = (double)(timestamps[i + 1] - timestamps[i]) / (double)m_Frequency;
						resolvedTimeStats[i / 2] = resolvedTime;
					}
				}

				start = m_MaxTimestampsCount * m_QueryFrameIndex;
				count = m_TimestampsCount[m_QueryFrameIndex];

				vkCmdResetQueryPool(VulkanContext::GetActiveCommandBuffer(), m_TimeQueryPool, start, count);
				m_TimestampsCount[m_QueryFrameIndex] = 0;

				m_CurrentTimeQueryIndex = 0;
			}

			// Reset pipeline query
			{
				uint32 start = m_MaxPipelineQueriesCount * Renderer::GetCurrentFrameIndex();
				uint32 count = m_PipelineQueriesCount[Renderer::GetCurrentFrameIndex()];

				if (count > 0)
				{
					vkGetQueryPoolResults(VulkanContext::GetLogicalDevice(),
						m_PipelineStatsQueryPool,
						start,
						count,
						count * sizeof(PipelineStatistics),
						m_ResolvedPipelineStats[Renderer::GetCurrentFrameIndex()].data(),
						sizeof(PipelineStatistics),
						VK_QUERY_RESULT_64_BIT);
				}

				start = m_MaxPipelineQueriesCount * m_QueryFrameIndex;
				count = m_PipelineQueriesCount[m_QueryFrameIndex];

				vkCmdResetQueryPool(VulkanContext::GetActiveCommandBuffer(), m_PipelineStatsQueryPool, start, count);
				m_PipelineQueriesCount[m_QueryFrameIndex] = 0;

				m_CurrentQueryPipelineIndex = 0;
			}
		});
	}

	void VulkanProfiler::BeginTimeQuery()
	{
		Renderer::Submit([this]()
		{
			uint32 start = m_MaxTimestampsCount * m_QueryFrameIndex;
			uint32 count = m_TimestampsCount[m_QueryFrameIndex];

			vkCmdWriteTimestamp(VulkanContext::GetActiveCommandBuffer(), VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, m_TimeQueryPool, start + count);

			m_TimestampsCount[m_QueryFrameIndex]++;
		});
	}

	double VulkanProfiler::EndTimeQuery()
	{
		Renderer::Submit([this]()
		{
			ATN_CORE_ASSERT(m_TimestampsCount[m_QueryFrameIndex] < m_MaxTimestampsCount, "Too much time queries per frame");

			uint32 start = m_MaxTimestampsCount * m_QueryFrameIndex;
			uint32 count = m_TimestampsCount[m_QueryFrameIndex];

			vkCmdWriteTimestamp(VulkanContext::GetActiveCommandBuffer(), VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, m_TimeQueryPool, start + count);

			m_TimestampsCount[m_QueryFrameIndex]++;
		});

		uint32 index = m_CurrentTimeQueryIndex;
		m_CurrentTimeQueryIndex++;

		return m_ResolvedTimeStats[Renderer::GetCurrentFrameIndex()][index];
	}

	void VulkanProfiler::BeginPipelineStatsQuery()
	{
		Renderer::Submit([this]()
		{
			uint32 start = m_MaxPipelineQueriesCount * m_QueryFrameIndex;
			uint32 count = m_PipelineQueriesCount[m_QueryFrameIndex];

			vkCmdBeginQuery(VulkanContext::GetActiveCommandBuffer(), m_PipelineStatsQueryPool, start + count, 0);
		});
	}

	const PipelineStatistics& VulkanProfiler::EndPipelineStatsQuery()
	{
		Renderer::Submit([this]()
		{
			ATN_CORE_ASSERT(m_PipelineQueriesCount[m_QueryFrameIndex] < m_MaxPipelineQueriesCount, "Too much pipeline queries per frame");

			uint32 start = m_MaxPipelineQueriesCount * m_QueryFrameIndex;
			uint32 count = m_PipelineQueriesCount[m_QueryFrameIndex];

			vkCmdEndQuery(VulkanContext::GetActiveCommandBuffer(), m_PipelineStatsQueryPool, start + count);

			m_PipelineQueriesCount[m_QueryFrameIndex]++;
		});

		uint32 index = m_CurrentQueryPipelineIndex;
		m_CurrentQueryPipelineIndex++;

		return m_ResolvedPipelineStats[Renderer::GetCurrentFrameIndex()][index];
	}
}
