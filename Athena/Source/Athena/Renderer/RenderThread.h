#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Thread.h"
#include "Athena/Core/CommandStream.h"

#include <concurrentqueue/concurrentqueue.h>

#include <mutex>
#include <atomic>


namespace Athena
{
	#define ENQUEUE_RENDER_COMMAND(...) RenderThread::Get().Enqueue((__VA_ARGS__))

	#define RENDER_THREAD_MAX_FRAME_LATENCY 1
	#define RENDER_THREAD_COMMAND_STREAMS_NUM (RENDER_THREAD_MAX_FRAME_LATENCY + 1)

	enum class RendererAPI
	{
		Empty = 0,
		Vulkan = 1,
		D3D12 = 2
	};

	enum class ShaderModel
	{
		SM_6_0,
		SM_6_1,
		SM_6_2,
		SM_6_3,
		SM_6_4,
		SM_6_5,
		SM_6_6,
		SM_6_7,
	};

	struct RendererConfig
	{
		RendererAPI API = RendererAPI::Vulkan;
		ShaderModel ShaderModel = ShaderModel::SM_6_3;
		FilePath ShaderSourcesPath;
	};

	class ATHENA_API RenderThread : public Thread
	{
	public:
		RenderThread();
		~RenderThread();

		static void Init(const RendererConfig& config);
		static void Shutdown();

		static RenderThread& Get() { return *s_Instance; }

		bool IsRHIInitialized() { return m_IsRHIInitialized; }

		template <typename FuncT>
		void Enqueue(FuncT&& func)
		{
			if (Bypass() || Thread::IsRenderThread())
			{
				func();
			}
			else if (Thread::IsGameThread())
			{
				m_CommandStreams[m_GameThreadCommandStreamIndex].Enqueue(std::forward<FuncT>(func));
			}
			else
			{
				m_AsyncCommands.enqueue(std::forward<FuncT>(func));
			}
		}

		void ExecuteAsyncCommands();

		void Kick();
		void Wait();

		bool Bypass() const;
 
	private:
		void RenderThreadMain();

	private:
		static inline RenderThread* s_Instance = nullptr;

		CommandStream m_CommandStreams[RENDER_THREAD_COMMAND_STREAMS_NUM];
		uint32 m_RenderThreadCommandStreamIndex = 0;
		uint32 m_GameThreadCommandStreamIndex = 0;

		moodycamel::ConcurrentQueue<std::move_only_function<void()>> m_AsyncCommands;
		moodycamel::ConsumerToken m_ConsumerToken;

		std::atomic<int32> m_RenderThreadFrameIndex = 0;
		std::atomic<int32> m_GameThreadFrameIndex = 0;
		std::atomic<bool> m_ShouldExit = false;
		std::atomic<bool> m_IsRHIInitialized = false;
	};


	namespace EnumUtils
	{
		inline const char* RendererAPIToString(RendererAPI api)
		{
			switch (api)
			{
			case RendererAPI::Empty: return "Empty";
			case RendererAPI::Vulkan: return "Vulkan";
			case RendererAPI::D3D12: return "D3D12";
			}
			
			checkf(false);
			return "";
		}

		inline const char* ShaderModelToString(ShaderModel model)
		{
			switch (model)
			{
			case ShaderModel::SM_6_0: return "SM_6_0";
			case ShaderModel::SM_6_1: return "SM_6_1";
			case ShaderModel::SM_6_2: return "SM_6_2";
			case ShaderModel::SM_6_3: return "SM_6_3";
			case ShaderModel::SM_6_4: return "SM_6_4";
			case ShaderModel::SM_6_5: return "SM_6_5";
			case ShaderModel::SM_6_6: return "SM_6_6";
			case ShaderModel::SM_6_7: return "SM_6_7";
			}

			checkf(false);
			return "";
		}
	}
}
