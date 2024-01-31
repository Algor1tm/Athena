#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Time.h"


namespace Athena
{
	struct PipelineStatistics
	{
		uint64 InputAssemblyVertices;
		uint64 InputAssemblyPrimitives;
		uint64 VertexShaderInvocations;
		uint64 GeometryShaderInvocations;
		uint64 GeometryShaderPrimitives;
		uint64 ClippingInvocations;
		uint64 ClippingPrimitives;
		uint64 FragmentShaderInvocations;
		uint64 ComputeShaderInvocations;
	};


	// Returns query results with FramesInFlight delay
	class ATHENA_API GPUProfiler : public RefCounted
	{
	public:
		static Ref<GPUProfiler> Create(uint32 maxTimestamps, uint32 maxPipelineQueries);
		virtual ~GPUProfiler() = default;

		virtual void Reset() = 0;

		virtual void BeginTimeQuery() = 0;
		virtual Time EndTimeQuery() = 0;

		virtual void BeginPipelineStatsQuery() = 0;
		virtual const PipelineStatistics& EndPipelineStatsQuery() = 0;
	};
}
