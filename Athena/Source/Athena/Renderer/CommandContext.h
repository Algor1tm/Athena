#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Buffer.h"
#include "Athena/Renderer/Color.h"
#include "Athena/Renderer/GPUBuffers.h"
#include "Athena/Renderer/Texture.h"
#include "Athena/Renderer/ResourceState.h"


namespace Athena
{
	enum class CommandContextType
	{
		Graphics = 0,
		AsyncCompute = 1,
		AsyncTransfer = 2,

		NumContexts = 3
	};

	class ATHENA_API CommandContext: public RefCounted
	{
	public:
		virtual ~CommandContext() = default;

		virtual CommandContextType GetType() = 0;

		virtual void Drain(CommandContextType srcContext, ResourceState waitDstStage) = 0;
		virtual void Flush() = 0;
		virtual void FlushAndWait() = 0;

		virtual void Transition(Texture* texture, ResourceState dst);
		virtual void Transition(Texture* texture, const TextureSubresourceRange& subresourceRange, ResourceState dst) = 0;
		virtual void Transition(GPUBuffer* buffer, ResourceState dst) = 0;
		virtual void TransitionOwnership(Texture* texture, CommandContextType dstContext, ResourceState dst);
		virtual void TransitionOwnership(Texture* texture, const TextureSubresourceRange& subresourceRange, CommandContextType dstContext, ResourceState dst) = 0;
		virtual void TransitionOwnership(GPUBuffer* buffer, CommandContextType dstContext, ResourceState dst) = 0;

		// Transfer operations
		virtual void UploadBuffer(GPUBuffer* buffer, const Buffer& data, uint64 offset = 0) = 0;
		virtual void UploadTexture(Texture* texture, const Buffer& data) = 0;

		// Debug
#if ATN_ENABLE_GPU_DEBUG
		virtual void BeginMarker(const String& name) = 0;
		virtual void EndMarker() = 0;
		virtual void BeginQueueMarker(const String& name) = 0;
		virtual void EndQueueMarker() = 0;
		virtual void InsertMarker(const String& name) = 0;
#endif

#if ATN_ENABLE_TRACY_GPU
		virtual void BeginTracyZone(uint32 line, std::string_view fileName, std::string_view functionName, const String& name) = 0;
		virtual void EndTracyZone() = 0;
#endif
	};


#if ATN_ENABLE_TRACY_GPU

	#define GPU_TRACY_BEGIN(ctx, name) ctx->BeginTracyZone(__LINE__, __FILE__, __FUNCTION__, name)
	#define GPU_TRACY_END(ctx) ctx->EndTracyZone()

	struct ContextTracyZoneScoped
	{
		ContextTracyZoneScoped(const Ref<CommandContext>& ctx, uint32 line, std::string_view fileName, std::string_view functionName, const String& name)
			: CmdContext(ctx)
		{
			CmdContext->BeginTracyZone(line, fileName, functionName, name);
		}

		~ContextTracyZoneScoped()
		{
			CmdContext->EndTracyZone();
		}

		Ref<CommandContext> CmdContext;
	};

	#define GPU_TRACY_SCOPED_IMPL(ctx, name, counter) ContextTracyZoneScoped _tracyGpuZone_##counter (ctx, __LINE__, __FILE__, __FUNCTION__, name)
	#define GPU_TRACY_SCOPED(ctx, name) GPU_TRACY_SCOPED_IMPL(ctx, name, __COUNTER__)
#else
	#define GPU_TRACY_BEGIN(ctx, name)
	#define GPU_TRACY_END(ctx)
	#define GPU_TRACY_SCOPED(ctx, name)
#endif


#if ATN_ENABLE_GPU_DEBUG

	#define GPU_INSERT_MARKER(ctx, name) ctx->InsertDebugLabel(label)
	
	#define GPU_MARKER_BEGIN(ctx, name) ctx->BeginMarker(name)
	#define GPU_MARKER_END(ctx) ctx->EndMarker()

	#define GPU_QUEUE_MARKER_BEGIN(ctx, name) ctx->BeginQueueMarker(name)
	#define GPU_QUEUE_MARKER_END(ctx) ctx->EndQueueMarker()


	struct ContextMarkerScoped
	{
		ContextMarkerScoped(const Ref<CommandContext>& ctx, const String& name)
			: CmdContext(ctx)
		{
			GPU_MARKER_BEGIN(CmdContext, name);
		}

		~ContextMarkerScoped()
		{
			GPU_MARKER_END(CmdContext);
		}

		Ref<CommandContext> CmdContext;
	};

	#define GPU_MARKER_SCOPED_IMPL(cmdList, name, counter) ContextMarkerScoped _ctxMarker_##counter (cmdList, name)
	#define GPU_MARKER_SCOPED(cmdList, name) GPU_MARKER_SCOPED_IMPL(cmdList, name, __COUNTER__)

	struct ContextQueueMarkerScoped
	{
		ContextQueueMarkerScoped(const Ref<CommandContext>& ctx, const String& name)
			: CmdContext(ctx)
		{
			GPU_QUEUE_MARKER_BEGIN(CmdContext, name);
		}

		~ContextQueueMarkerScoped()
		{
			GPU_QUEUE_MARKER_END(CmdContext);
		}

		Ref<CommandContext> CmdContext;
	};

	#define GPU_QUEUE_MARKER_SCOPED_IMPL(cmdList, name, counter) ContextQueueMarkerScoped _ctxQueueMarker_##counter (cmdList, name)
	#define GPU_QUEUE_MARKER_SCOPED(cmdList, name) GPU_MARKER_SCOPED_IMPL(cmdList, name, __COUNTER__)

#else 
	#define GPU_INSERT_MARKER(ctx, name)
	#define GPU_MARKER_BEGIN(ctx, name)
	#define GPU_MARKER_END(ctx)
	#define GPU_QUEUE_MARKER_BEGIN(ctx, name)
	#define GPU_QUEUE_MARKER_END(ctx)
	#define GPU_MARKER_SCOPED(cmdList, name)
	#define GPU_QUEUE_MARKER_SCOPED(cmdList, name)
#endif

#define GPU_EVENT(ctx, name) GPU_TRACY_SCOPED(ctx, name); GPU_MARKER_SCOPED(ctx, name); 


	namespace EnumUtils
	{
		inline const char* CmdContextTypeToString(CommandContextType type)
		{
			switch (type)
			{
			case CommandContextType::Graphics: return "Graphics";
			case CommandContextType::AsyncCompute: return "AsyncCompute";
			case CommandContextType::AsyncTransfer: return "AsyncTransfer";
			}

			checkf(false);
			return "Unknown";
		}
	}
}
