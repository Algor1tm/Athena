#pragma once 

#include "Athena/Core/Core.h"
#include "Athena/Renderer/RHI/RHI.h"
#include "Athena/Renderer/RHI/Vulkan/CommandBufferManager.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanAllocator.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanConfiguration.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanDebug.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanDevice.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	struct InstanceExtensionsInfo
	{
		bool GetPhysicalDeviceProperties2EXTSupported;
#if VULKAN_ENABLE_DEBUG_INFO
		bool DebugReportEXTSupported;
		VkDebugReportCallbackEXT DebugReportCallback;
#endif
	};

	class ATHENA_API VulkanRHI : public RHI
	{
		// Interface functions
	public:
		virtual void Init(const RendererConfig& config) override;

		virtual void Shutdown() override;

		virtual void BeginFrame(const Ref<Window>& window) override;

		virtual void EndFrame(const Ref<Window>& window) override;

		virtual Ref<VertexBuffer> CreateVertexBuffer() override;

		virtual Ref<IndexBuffer> CreateIndexBuffer() override;

		virtual Ref<CommandList> CreateCommandList() override;

		virtual Ref<SwapChain> CreateSwapChain(void* windowHandle, bool vsync) override;

		virtual uint64 GetMemoryUsage() override;

		virtual const GPUCapabilities& GetGPUCapabilities() override;

		virtual const FormatProperties& GetFormatProperties(Format format) override;

		// Vulkan functions
	public:
		static VulkanRHI* Get()
		{
			return (VulkanRHI*)RHI::Get();
		}

		VkInstance GetVulkanInstance() const { return m_Instance; }

		VulkanDevice* GetDevice() { return &m_Device; }

		const InstanceExtensionsInfo& GetExtensionsInfo() const { return m_ExtensionsInfo; }

		Ref<VulkanAllocator> GetAllocator() const { return m_Allocator; }

		CommandBufferManager* GetCmdBufferManager() { return &m_CmdBufferManager; }

		uint32 GetVulkanVersion() const { return m_VulkanVersion; }

		template <typename FuncT>
		void DeleteFrameResourcesDeferred(FuncT&& func)
		{
			m_ResourceFreeQueues[m_FrameResourcesIndex].Submit(std::forward<FuncT>(func));
		}

		uint32 GetFrameResourcesIndex() const
		{
			return m_FrameResourcesIndex;
		}

	private:
		void QueryInstanceLayers();
		void QueryInstanceExtensions();

	private:
		VkInstance m_Instance;
		InstanceExtensionsInfo m_ExtensionsInfo;
		VulkanDevice m_Device;
		Ref<VulkanAllocator> m_Allocator;
		CommandBufferManager m_CmdBufferManager;

		uint32 m_VulkanVersion;
		std::vector<const char*> m_InstanceLayers;
		std::vector<const char*> m_InstanceExtensions;

		uint32 m_FrameResourcesIndex = 0;
		std::vector<CommandStream> m_ResourceFreeQueues;
	};
}
