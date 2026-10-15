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

	public:
		CommandBufferState State;
		VkCommandBuffer CommandBuffer;
		CommandStream OnCompletionCallbacks;
		VkFence SignalFence;
		Time LastIdleTime;
	};

	class CommandBufferManager
	{
	public:
		void Init();
		void Destroy();

		VulkanCommandBuffer* GetCommandBuffer();
		VulkanCommandBuffer* GetUploadCommandBuffer();

		void Submit(VulkanCommandBuffer* commandBuffer, bool wait = false);
		void SubmitUpload();

		void OnUpdate();

	private:
		void OnCompletion(VulkanCommandBuffer* commandBuffer);
		VulkanCommandBuffer* FindFreeCommandBuffer();
		VulkanCommandBuffer* AllocateCommandBuffer();

	private:
		VkCommandPool m_Pool = VK_NULL_HANDLE;
		std::vector<VulkanCommandBuffer*> m_CommandBuffers;
		VulkanCommandBuffer* m_UploadCommandBuffer = nullptr;
	};
}
