#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Color.h"


namespace Athena
{
	class ATHENA_API CommandList: public RefCounted
	{
	public:
		static Ref<CommandList> Create();
		virtual ~CommandList() = default;

		virtual void BeginDebugLabel(const String& name, const IntegerColor& color = IntegerColor::Black) = 0;
		virtual void EndDebugLabel() = 0;
		virtual void InsertDebugLabel(const String& name, const IntegerColor& color = IntegerColor::Black) = 0;

		virtual void Submit(bool wait = false) = 0;
	};


	class ATHENA_API TracyGPUZone: public RefCounted
	{
	public:
		virtual ~TracyGPUZone() = default;
	};


#if ATN_ENABLE_TRACY_GPU
	struct ATHENA_API TracyGPUZoneScoped
	{
		TracyGPUZoneScoped(const Ref<CommandList>& cmdList, uint32 line, std::string_view fileName, std::string_view functionName, const String& name);
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
		CommandListLabelScoped(const Ref<CommandList>& cmdList, const String& label, const IntegerColor& color = IntegerColor::Black)
			: CommandList(cmdList)
		{
			CommandList->BeginDebugLabel(label, color);
		}

		~CommandListLabelScoped()
		{
			CommandList->EndDebugLabel();
		}

		Ref<CommandList> CommandList;
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
}
