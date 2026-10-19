#include "RenderThread.h"

#include "Athena/Core/ConsoleManager.h"
#include "Athena/Core/Stats.h"
#include "Athena/Renderer/EngineRenderAssets.h"
#include "Athena/Renderer/RHI/RHI.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanRHI.h"


namespace Athena
{
	DEFINE_STATS_GROUP("Renderer Stats", STATGROUP_RendererStats, StatsThread::RenderThread);
	DEFINE_CYCLE_STAT("CPU Wait", STAT_CPUWait, STATGROUP_RendererStats);

	static AutoCVar<int32> CVarRenderThreadCommandStreamSize(
		"r.RenderThreadCommandStreamSize",
		1024 * 1024,
		"Set render thread command stream size in bytes (default - 1MB)."
	);

	static AutoCVar<bool> CVarEnableRenderThread(
		"r.EnableRenderThread",
		false,
		"Enables or disables render thread (default - true)."
);

	RenderThread::RenderThread()
		: Thread("RenderThread", [this]() { RenderThreadMain(); })
	{
		m_RenderThreadCommandStreamIndex = 0;
		m_GameThreadCommandStreamIndex = 0;

		if (!Bypass())
		{
			for (uint32 i = 0; i < RENDER_THREAD_COMMAND_STREAMS_NUM; ++i)
			{
				m_CommandStreams[i] = std::forward<CommandStream>(CommandStream(CVarRenderThreadCommandStreamSize.GetInt()));
			}

			ATN_LOG_INFO(Renderer, "Initialized Render Thread");
		}

		Thread::s_RenderThreadID = GetID();

		Start();
	}

	RenderThread::~RenderThread()
	{

	}

	void RenderThread::Init(const RendererConfig& config)
	{
		s_Instance = new RenderThread();

		switch(config.API)
		{
			case RendererAPI::Vulkan: RHI::s_RHI = new VulkanRHI(); break;
			//case RendererAPI::D3D12:  RHI::s_RHI = new D3D12RHI(); break;
		}

		ENQUEUE_RENDER_COMMAND([config]()
		{
			RHI::s_RHI->Init(config);
			s_Instance->m_IsRHIInitialized = true;
		});

		Font::Init();
		EngineRenderAssets::Get().Init();

		// TODO: move to task manager 
	}

	void RenderThread::Shutdown()
	{
		EngineRenderAssets::Get().Destroy();
		Font::Shutdown();

		ENQUEUE_RENDER_COMMAND([]()
		{
			RHI::s_RHI->Shutdown();
		});

		s_Instance->m_ShouldExit = true;
		s_Instance->Join();

		delete RHI::s_RHI;
		delete s_Instance;
	}

	void RenderThread::RenderThreadMain()
	{
		if (Bypass())
			return;

		while (!m_ShouldExit)
		{
			TRACY_PROFILE_SCOPE("RenderThreadFrame");
			STATS_THREAD_HEARTBEAT(StatsThread::RenderThread);

			{
				TRACY_PROFILE_SCOPE("WaitForGameThread");

				// Wait for game thread, there should be at least 1 frame difference
				while (m_RenderThreadFrameIndex + 1 > m_GameThreadFrameIndex)
				{
					Thread::YieldExecution();
					continue;
				}
			}

			{
				TRACY_PROFILE_SCOPE("ExecuteCommands");
				m_CommandStreams[m_RenderThreadCommandStreamIndex].Flush();
			}

			m_RenderThreadCommandStreamIndex = (m_RenderThreadCommandStreamIndex + 1) % RENDER_THREAD_COMMAND_STREAMS_NUM;
			m_RenderThreadFrameIndex++;
		}

		Thread::s_RenderThreadID = 0;
	}
	
	void RenderThread::Kick()
	{
		if (Bypass())
			return;

		std::unique_lock<std::mutex> lock(m_EnqueueMutex);

		m_GameThreadCommandStreamIndex = (m_GameThreadCommandStreamIndex + 1) % RENDER_THREAD_COMMAND_STREAMS_NUM;
		m_GameThreadFrameIndex++;
	}

	void RenderThread::Wait()
	{
		if (Bypass())
			return;

		TRACY_PROFILE_SCOPE("WaitForRenderThread");

		// Wait for render thread here
		while (m_RenderThreadFrameIndex + RENDER_THREAD_MAX_FRAME_LATENCY < m_GameThreadFrameIndex)
		{
			Thread::YieldExecution();
			continue;
		}
	}

	bool RenderThread::Bypass() const
	{
		return !CVarEnableRenderThread.GetBool();
	}
}
