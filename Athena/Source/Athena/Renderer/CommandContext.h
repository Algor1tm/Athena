#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Buffer.h"
#include "Athena/Renderer/Color.h"
#include "Athena/Renderer/GPUBuffers.h"
#include "Athena/Renderer/Texture.h"


namespace Athena
{
	enum class CommandContextType
	{
		Graphics = 0,
		AsyncCompute = 1,
		AsyncTransfer = 2,

		NumContexts = 3
	};

	enum PipelineAccess
	{
		PA_Unknown = BIT(0),

		// Read states
		PA_CopySrc			= BIT(1),
		PA_VertexInput		= BIT(2),
		PA_SRVVertexShader = BIT(3),
		PA_SRVPixelShader  = BIT(4),
		PA_SRVCompute		= BIT(5),
		PA_DSVRead			= BIT(6),

		PA_SRVGraphics = PA_SRVVertexShader | PA_SRVPixelShader,
		PA_SRVMask = PA_SRVCompute | PA_SRVGraphics,

		// Read-Write states
		PA_CopyDst  = BIT(7),
		PA_UAV		 = BIT(8),
		PA_RTV		 = BIT(9),
		PA_DSVWrite = BIT(10),
	};

	class ATHENA_API CommandContext: public RefCounted
	{
	public:
		virtual ~CommandContext() = default;

		virtual CommandContextType GetType() = 0;

		virtual void Drain(CommandContextType srcContext, PipelineAccess waitDstStage) = 0;
		virtual bool Flush() = 0;
		virtual bool FlushAndWait() = 0;

		virtual void Transition(Texture* texture, PipelineAccess src, PipelineAccess dst);
		virtual void Transition(Texture* texture, const TextureRange& subresource, PipelineAccess src, PipelineAccess dst) = 0;
		virtual void Transition(GPUBuffer* buffer, PipelineAccess src, PipelineAccess dst) = 0;
		virtual void TransitionOwnership(Texture* texture, PipelineAccess src, CommandContextType dstContext, PipelineAccess dst);
		virtual void TransitionOwnership(Texture* texture, const TextureRange& subresource, PipelineAccess src, CommandContextType dstContext, PipelineAccess dst) = 0;
		virtual void TransitionOwnership(GPUBuffer* buffer, PipelineAccess src, CommandContextType dstContext, PipelineAccess dst) = 0;

		// Transfer operations
		virtual void UploadBuffer(GPUBuffer* buffer, const Buffer& data, uint64 offset = 0) = 0;
		virtual void UploadTexture(Texture* texture, const Buffer& data) = 0;

		// Debug
		virtual void BeginDebugLabel(const String& name, const IntegerColor& color = IntegerColor::Black) = 0;
		virtual void EndDebugLabel() = 0;
		virtual void InsertDebugLabel(const String& name, const IntegerColor& color = IntegerColor::Black) = 0;
		virtual bool HasTracyContext() = 0;
	};


	class ATHENA_API TracyGPUZone: public RefCounted
	{
	public:
		virtual ~TracyGPUZone() = default;
	};


#if ATN_ENABLE_TRACY_GPU
	struct ATHENA_API TracyGPUZoneScoped
	{
		TracyGPUZoneScoped(const Ref<CommandContext>& cmdCtx, uint32 line, std::string_view fileName, std::string_view functionName, const String& name);
		~TracyGPUZoneScoped();

		Ref<TracyGPUZone> TracyZone;
	};

	#define TRACY_GPU_SCOPED_IMPL(cmdList, name, counter) TracyGPUZoneScoped _tracyGpuZone_##counter (cmdList, __LINE__, __FILE__, __FUNCTION__, name)
	#define TRACY_GPU_SCOPED(cmdList, name) TRACY_GPU_SCOPE_IMPL(cmdList, name, __COUNTER__)
#else
	#define TRACY_GPU_SCOPE(cmdList, name)
#endif


#if ATN_ENABLE_GPU_DEBUG
	struct CommandListLabelScoped
	{
		CommandListLabelScoped(const Ref<CommandContext>& cmdCtx, const String& label, const IntegerColor& color = IntegerColor::Black)
			: CmdContext(cmdCtx)
		{
			CmdContext->BeginDebugLabel(label, color);
		}

		~CommandListLabelScoped()
		{
			CmdContext->EndDebugLabel();
		}

		Ref<CommandContext> CmdContext;
	};

	#define GPU_LABEL_SCOPED_IMPL(cmdList, label, color, counter) CommandListLabelScoped _cmdListLabel_##counter (cmdList, label, color)
	#define GPU_LABEL_SCOPED(cmdList, label) GPU_LABEL_SCOPED_IMPL(cmdList, label, IntegerColor::Black, __COUNTER__)
	#define GPU_LABEL_SCOPED_C(cmdList, label, color) GPU_LABEL_SCOPED_IMPL(cmdList, label, color, __COUNTER__)

	#define GPU_INSERT_LABEL(cmdList, label) cmdList->InsertDebugLabel(label)
	#define GPU_INSERT_LABEL_C(cmdList, label, color) cmdList->InsertDebugLabel(label, color)

#else 
	#define GPU_LABEL_SCOPED(cmdList, label)
	#define GPU_LABEL_SCOPED_C(cmdList, label, color)
	
	#define GPU_INSERT_LABEL(cmdList, label)
	#define GPU_INSERT_LABEL_C(cmdList, label, color)
#endif

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
