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
		VulkanProfiler();
		~VulkanProfiler();

		virtual void Reset() override;

		virtual void BeginTimeQuery() override;
		virtual double EndTimeQuery() override;

		virtual void BeginPipelineStatsQuery() override;
		virtual const PipelineStatistics& EndPipelineStatsQuery() override;

	private:
		static constexpr uint32 m_MaxTimestampsCount = 128;
		static constexpr uint32 m_MaxPipelineQueriesCount = 2;

	private:
		uint32 m_QueryFrameIndex;

		VkQueryPool m_TimeQueryPool;
		double m_Frequency;
		std::vector<uint32> m_TimestampsCount;
		std::vector<std::array<uint64, m_MaxTimestampsCount>> m_Timestamps;
		std::vector<std::array<double, m_MaxTimestampsCount / 2>> m_ResolvedTimeStats;
		uint32 m_CurrentTimeQueryIndex;

		VkQueryPool m_PipelineStatsQueryPool;
		std::vector<uint32> m_PipelineQueriesCount;
		std::vector<std::array<PipelineStatistics, m_MaxPipelineQueriesCount>> m_ResolvedPipelineStats;
		uint32 m_CurrentQueryPipelineIndex;
	};
}