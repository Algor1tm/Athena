#include "RenderThread.h"

#include "Athena/Core/ConsoleManager.h"
#include "Athena/Renderer/RHI/RHI.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanRHI.h"


namespace Athena
{
	static AutoCVar<int32> CVarRenderThreadCommandStreamSize(
		"r.RenderThreadCommandStreamSize",
		1024 * 1024,
		"Set render thread command stream size in bytes (default - 1MB)."
	);


	RenderThread::RenderThread()
		: Thread("RenderThread", [this]() { RenderThreadMain(); })
	{
		for (uint32 i = 0; i < RENDER_THREAD_COMMAND_STREAMS_NUM; ++i)
		{
			m_CommandStreams[i] = std::forward<CommandStream>(CommandStream(CVarRenderThreadCommandStreamSize.GetInt()));
		}

		m_RenderThreadCommandStreamIndex = 0;
		m_GameThreadCommandStreamIndex = 1;

		ATN_LOG_INFO(Renderer, "Initialized Render Thread");
		Start();
	}

	RenderThread::~RenderThread()
	{

	}

	void RenderThread::Init(const RendererConfig& config)
	{
		switch(config.API)
		{
			case RendererAPI::Vulkan: RHI::s_RHI = new VulkanRHI(); break;
			//case RendererAPI::D3D12:  RHI::s_RHI = new D3D12RHI(); break;
		}

		// Initialize RHI on game thread to safely access gpu properties on game thread
		RHI::s_RHI->Init(config);

		s_Instance = new RenderThread();

		// TODO: move to task manager 
		Thread::s_GameThreadID = Thread::GetCurrentThreadID();
		Thread::s_RenderThreadID = s_Instance->GetID();
	}

	void RenderThread::Shutdown()
	{
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
		while (!m_ShouldExit)
		{
			// Wait for game thread, there should be at least 1 frame difference
			while (m_RenderThreadFrameIndex + 1 > m_GameThreadFrameIndex)
			{
				Thread::YieldExecution();
				continue;
			}

			m_CommandStreams[m_RenderThreadCommandStreamIndex].Flush();

			m_RenderThreadCommandStreamIndex = (m_RenderThreadCommandStreamIndex + 1) % RENDER_THREAD_COMMAND_STREAMS_NUM;
			m_RenderThreadFrameIndex++;
		}
	}
	
	void RenderThread::Kick()
	{
		m_GameThreadCommandStreamIndex = (m_GameThreadCommandStreamIndex + 1) % RENDER_THREAD_COMMAND_STREAMS_NUM;
		m_GameThreadFrameIndex++;
	}

	void RenderThread::Wait()
	{
		// Wait for render thread here
		while (m_RenderThreadFrameIndex + RENDER_THREAD_MAX_FRAME_LATENCY < m_GameThreadFrameIndex)
		{
			Thread::YieldExecution();
			continue;
		}
	}
}
