#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/CommandStream.h"
#include "Athena/Core/Time.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	class VulkanCommandContext;

	enum class CommandBufferState
	{
		Idle,
		RecordingCommands,
		Submitted,
	};

	class VulkanCommandBuffer
	{
	public:
		VulkanCommandBuffer(VulkanCommandContext* context);
		~VulkanCommandBuffer();

		void AddDependency(VkSemaphore semaphore, uint64 value, VkPipelineStageFlags2 waitDstStage);

		template <typename FuncT>
		void AddCallbackOnCompletion(FuncT&& func)
		{
			OnCompletionCallbacks.Enqueue(std::forward<FuncT>(func));
		}

		void WaitCompletion();
		void OnCompletion();

		void Begin();
		void End();

		void Submit(VkQueue queue, VkSemaphoreSubmitInfo* semaphoreSignalInfo = nullptr);

	public:
		CommandBufferState State = CommandBufferState::Idle;
		VkCommandBuffer CmdBuffer = VK_NULL_HANDLE;
		VulkanCommandContext* Context = nullptr;
		CommandStream OnCompletionCallbacks;
		VkFence SignalFence = VK_NULL_HANDLE;
		std::vector<VkSemaphoreSubmitInfo> WaitSemaphores;
		Time LastUsedTime;
	};
}
