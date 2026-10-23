#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Thread.h"
#include "Athena/Core/CommandStream.h"

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

	struct RendererConfig
	{
		RendererAPI API = RendererAPI::Vulkan;
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
				return;
			}
			
			// TODO: mutex is bad here probably, but i need to push commands from multiple threads
			std::unique_lock<std::mutex> lock(m_EnqueueMutex);
			m_CommandStreams[m_GameThreadCommandStreamIndex].Submit(std::forward<FuncT>(func));
		}

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

		std::mutex m_EnqueueMutex;
		std::atomic<int32> m_RenderThreadFrameIndex = 0;
		std::atomic<int32> m_GameThreadFrameIndex = 0;
		std::atomic<bool> m_ShouldExit = false;
		std::atomic<bool> m_IsRHIInitialized = false;
	};
}
