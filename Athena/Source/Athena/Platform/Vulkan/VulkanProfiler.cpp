#include "VulkanProfiler.h"

#include "Athena/Platform/Vulkan/VulkanUtils.h"


namespace Athena
{
	VulkanProfiler::VulkanProfiler(uint32 maxTimestamps, uint32 maxPipelineQueries)
	{
		m_MaxTimestampsCount = maxTimestamps;
		m_MaxPipelineQueriesCount = maxPipelineQueries;

		// Create time queury pool
		{
			VkQueryPoolCreateInfo queryPoolInfo = {};
			queryPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
			queryPoolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
			queryPoolInfo.queryCount = m_MaxTimestampsCount * Renderer::GetFramesInFlight();
				
			VK_CHECK(vkCreateQueryPool(VulkanContext::GetLogicalDevice(), &queryPoolInfo, nullptr, &m_TimeQueryPool));
				
			vkResetQueryPool(VulkanContext::GetLogicalDevice(), m_TimeQueryPool, 0, queryPoolInfo.queryCount);
				
			m_TimestampsCount.resize(Renderer::GetFramesInFlight());

			m_Timestamps.resize(Renderer::GetFramesInFlight());
			for (auto& timestamps : m_Timestamps)
				timestamps.resize(m_MaxTimestampsCount);

			m_ResolvedTimeStats.resize(Renderer::GetFramesInFlight());
			for (auto& stats : m_ResolvedTimeStats)
				stats.resize(m_MaxTimestampsCount / 2);

			m_Frequency = 1000000.0 / (double)Renderer::GetRenderCaps().TimestampPeriod;
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

			VK_CHECK(vkCreateQueryPool(VulkanContext::GetLogicalDevice(), &queryPoolInfo, nullptr, &m_PipelineStatsQueryPool));

			vkResetQueryPool(VulkanContext::GetLogicalDevice(), m_PipelineStatsQueryPool, 0, queryPoolInfo.queryCount);

			m_PipelineQueriesCount.resize(Renderer::GetFramesInFlight());
			m_ResolvedPipelineStats.resize(Renderer::GetFramesInFlight());
			for (auto& stats : m_ResolvedPipelineStats)
				stats.resize(m_MaxPipelineQueriesCount);
		}
	}

	VulkanProfiler::~VulkanProfiler()
	{
		Renderer::SubmitResourceFree([timeQueryPool = m_TimeQueryPool, pipelineQueryPool = m_PipelineStatsQueryPool]()
		{
			vkDestroyQueryPool(VulkanContext::GetLogicalDevice(), timeQueryPool, nullptr);
			vkDestroyQueryPool(VulkanContext::GetLogicalDevice(), pipelineQueryPool, nullptr);
		});
	}

	void VulkanProfiler::Reset()
	{
		Renderer::Submit([this]()
		{
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
						Time resolvedTime = Time::Microseconds((double)(timestamps[i + 1] - timestamps[i]) / (double)m_Frequency);
						resolvedTimeStats[i / 2] = resolvedTime;
					}
				}

				vkCmdResetQueryPool(VulkanContext::GetActiveCommandBuffer(), m_TimeQueryPool, start, count);

				m_TimestampsCount[Renderer::GetCurrentFrameIndex()] = 0;
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

				vkCmdResetQueryPool(VulkanContext::GetActiveCommandBuffer(), m_PipelineStatsQueryPool, start, count);

				m_PipelineQueriesCount[Renderer::GetCurrentFrameIndex()] = 0;
				m_CurrentPipelineQueryIndex = 0;
			}
		});
	}

	void VulkanProfiler::BeginTimeQuery()
	{
		Renderer::Submit([this]()
		{
			uint32 start = m_MaxTimestampsCount * Renderer::GetCurrentFrameIndex();
			uint32 count = m_TimestampsCount[Renderer::GetCurrentFrameIndex()];

			vkCmdWriteTimestamp(VulkanContext::GetActiveCommandBuffer(), VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, m_TimeQueryPool, start + count);

			m_TimestampsCount[Renderer::GetCurrentFrameIndex()]++;
		});
	}

	Time VulkanProfiler::EndTimeQuery()
	{
		Renderer::Submit([this]()
		{
			ATN_CORE_ASSERT(m_TimestampsCount[Renderer::GetCurrentFrameIndex()] < m_MaxTimestampsCount, "Too much time queries per frame");

			uint32 start = m_MaxTimestampsCount * Renderer::GetCurrentFrameIndex();
			uint32 count = m_TimestampsCount[Renderer::GetCurrentFrameIndex()];

			vkCmdWriteTimestamp(VulkanContext::GetActiveCommandBuffer(), VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, m_TimeQueryPool, start + count);

			m_TimestampsCount[Renderer::GetCurrentFrameIndex()]++;
		});

		uint32 index = m_CurrentTimeQueryIndex;
		m_CurrentTimeQueryIndex++;

		return m_ResolvedTimeStats[Renderer::GetCurrentFrameIndex()][index];
	}

	void VulkanProfiler::BeginPipelineStatsQuery()
	{
		Renderer::Submit([this]()
		{
			uint32 start = m_MaxPipelineQueriesCount * Renderer::GetCurrentFrameIndex();
			uint32 count = m_PipelineQueriesCount[Renderer::GetCurrentFrameIndex()];

			vkCmdBeginQuery(VulkanContext::GetActiveCommandBuffer(), m_PipelineStatsQueryPool, start + count, 0);
		});
	}

	const PipelineStatistics& VulkanProfiler::EndPipelineStatsQuery()
	{
		Renderer::Submit([this]()
		{
			ATN_CORE_ASSERT(m_PipelineQueriesCount[Renderer::GetCurrentFrameIndex()] < m_MaxPipelineQueriesCount, "Too much pipeline queries per frame");

			uint32 start = m_MaxPipelineQueriesCount * Renderer::GetCurrentFrameIndex();
			uint32 count = m_PipelineQueriesCount[Renderer::GetCurrentFrameIndex()];

			vkCmdEndQuery(VulkanContext::GetActiveCommandBuffer(), m_PipelineStatsQueryPool, start + count);

			m_PipelineQueriesCount[Renderer::GetCurrentFrameIndex()]++;
		});

		uint32 index = m_CurrentPipelineQueryIndex;
		m_CurrentPipelineQueryIndex++;

		return m_ResolvedPipelineStats[Renderer::GetCurrentFrameIndex()][index];
	}
}
