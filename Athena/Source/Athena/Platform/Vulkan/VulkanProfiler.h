#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Renderer.h"
#include "Athena/Renderer/GPUProfiler.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	
	class VulkanProfiler : public GPUProfiler
	{
	public:
		VulkanProfiler(const GPUProfilerCreateInfo& info);
		~VulkanProfiler();

		virtual void Reset() override;

		virtual void BeginTimeQuery() override;
		virtual Time EndTimeQuery() override;

		virtual void BeginPipelineStatsQuery() override;
		virtual const PipelineStatistics& EndPipelineStatsQuery() override;

	private:
		VkQueryPool m_TimeQueryPool;
		double m_Frequency;
		std::vector<uint32> m_TimestampsCount;
		std::vector<std::vector<uint64>> m_Timestamps;
		std::vector<std::vector<Time>> m_ResolvedTimeStats;
		uint32 m_CurrentTimeQueryIndex = 0;

		VkQueryPool m_PipelineStatsQueryPool;
		std::vector<uint32> m_PipelineQueriesCount;
		std::vector<std::vector<PipelineStatistics>> m_ResolvedPipelineStats;
		uint32 m_CurrentPipelineQueryIndex = 0;
	};
}