#include "RenderThread.h"

#include "Athena/Core/ConsoleManager.h"
#include "Athena/Core/Stats.h"
#include "Athena/Renderer/Shader.h"
#include "Athena/Renderer/EngineRenderAssets.h"
#include "Athena/Renderer/RHI/RHI.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanRHI.h"
#include "Athena/Renderer/RHI/D3D12/D3D12RHI.h"


namespace Athena
{
	DEFINE_STATS_GROUP("RenderThread Stats", STATGROUP_RenderThreadStats, ST_RenderThread);

	DEFINE_CYCLE_STAT("Wait for GameThread", STAT_WaitForGameThread, STATGROUP_RenderThreadStats);
	DEFINE_CYCLE_STAT("Wait for GPU", STAT_WaitForGPU, STATGROUP_RenderThreadStats);

	EXTERN_CYCLE_STAT(STAT_WaitForRenderThread);

	static AutoCVar<bool> CVarEnableRenderThread(
		"r.EnableRenderThread",
		true,
		"Enables or disables render thread (default - true)."
	);

	RenderThread::RenderThread()
		: Thread("RenderThread", [this]() { RenderThreadMain(); }),
		  m_ConsumerToken(m_AsyncCommands)
	{
		m_RenderThreadCommandStreamIndex = 0;
		m_GameThreadCommandStreamIndex = 0;

		m_RenderThreadFrameIndex = 0;
		m_GameThreadFrameIndex = 0;

		if (!Bypass())
		{
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
			case RendererAPI::D3D12:  RHI::s_RHI = new D3D12RHI(); break;
		}

		ENQUEUE_RENDER_COMMAND([config]()
		{
			RHI::s_RHI->Init(config);
			s_Instance->m_IsRHIInitialized = true;

			ShaderManager::Get().Init();
			ShaderManager::Get().CompileAll();
		});

		Font::Init();
		EngineRenderAssets::Get().Init();
	}

	void RenderThread::Shutdown()
	{
		EngineRenderAssets::Get().Destroy();
		Font::Shutdown();

		ENQUEUE_RENDER_COMMAND([]()
		{
			ShaderManager::Get().Destroy();
			RHI::s_RHI->Shutdown();
		});

		s_Instance->m_ShouldExit = true;
		s_Instance->Kick();
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
			STATS_THREAD_HEARTBEAT(ST_RenderThread);

			{
				TRACY_PROFILE_SCOPE("WaitForGameThread");
				SCOPE_CYCLE_STAT(STAT_WaitForGameThread);

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

	void RenderThread::ExecuteAsyncCommands()
	{
		TRACY_PROFILE_FUNC();

		uint32 size = m_AsyncCommands.size_approx();
		if (size == 0)
			return;
		
		std::vector<std::move_only_function<void()>> commands(size);
		uint32 actualSize = m_AsyncCommands.try_dequeue_bulk(m_ConsumerToken, commands.data(), commands.size());

		for (uint32 i = 0; i < actualSize; ++i)
		{
			commands[i]();
		}
	}
	
	void RenderThread::Kick()
	{
		if (Bypass())
			return;

		m_GameThreadCommandStreamIndex = (m_GameThreadCommandStreamIndex + 1) % RENDER_THREAD_COMMAND_STREAMS_NUM;
		m_GameThreadFrameIndex++;
	}

	void RenderThread::Wait()
	{
		if (Bypass())
			return;

		TRACY_PROFILE_SCOPE("WaitForRenderThread");
		SCOPE_CYCLE_STAT(STAT_WaitForRenderThread);

		// Wait for render thread here
		while (m_RenderThreadFrameIndex + RENDER_THREAD_MAX_FRAME_LATENCY <= m_GameThreadFrameIndex)
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
