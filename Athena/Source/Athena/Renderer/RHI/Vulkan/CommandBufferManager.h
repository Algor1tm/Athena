#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Time.h"
#include "Athena/Core/CommandStream.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	enum class CommandBufferState
	{
		Idle,
		RecordingCommands,
		Submitted,
	};

	class VulkanCommandBuffer
	{
	public:
		VulkanCommandBuffer(VkCommandPool pool);
		~VulkanCommandBuffer();

		template <typename FuncT>
		void AddCallbackOnCompletion(FuncT&& func)
		{
			OnCompletionCallbacks.Submit(std::forward<FuncT>(func));
		}

		void Begin();
		void End();

		void OnCompletion();

	public:
		CommandBufferState State;
		VkCommandBuffer CommandBuffer;
		CommandStream OnCompletionCallbacks;
		VkFence SignalFence;
		Time LastUsedTime;
	};

	class CommandBufferManager
	{
	public:
		void Init();
		void Destroy();

		VulkanCommandBuffer* GetCommandBuffer();
		VulkanCommandBuffer* GetTransferCommandBuffer();

		VulkanCommandBuffer* GetPresentCommandBuffer();
		VkSemaphore	GetPresentCmdBufferSemaphore();

		void WaitForCmdBuffer(VulkanCommandBuffer* commandBuffer);

		void Submit(VulkanCommandBuffer* commandBuffer);
		void SubmitTransfer();
		void SubmitForPresent(VkSemaphore imageAcquiredSemaphore, VkPipelineStageFlags flags);

		void NewFrame();

	private:
		VulkanCommandBuffer* FindFreeCommandBuffer();
		VulkanCommandBuffer* AllocateCommandBuffer();

	private:
		VkCommandPool m_Pool = VK_NULL_HANDLE;
		std::vector<VulkanCommandBuffer*> m_CommandBuffers;
		VulkanCommandBuffer* m_TransferCommandBuffer = nullptr;

		std::vector<VulkanCommandBuffer*> m_PresentCommandBuffers;
		std::vector<VkSemaphore> m_PresentCmdBufferSemaphores;
	};
}
