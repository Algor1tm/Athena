#pragma once 

#include "Athena/Core/Core.h"
#include "Athena/Core/CommandStream.h"
#include "Athena/Renderer/RHI/RHI.h"
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
		bool DebugUtilsEXTSupported;
		VkDebugUtilsMessengerEXT DebugUtilsMessenger;
		PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT;
		PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT;
		PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT;
		PFN_vkCmdInsertDebugUtilsLabelEXT vkCmdInsertDebugUtilsLabelEXT;
		PFN_vkQueueEndDebugUtilsLabelEXT vkQueueEndDebugUtilsLabelEXT;
		PFN_vkQueueBeginDebugUtilsLabelEXT vkQueueBeginDebugUtilsLabelEXT;
#endif
	};

	class ATHENA_API VulkanRHI : public RHI
	{
		// Interface functions
	public:
		VulkanRHI() = default;

		// TODO: idk why and where it tries to copy vulkanrhi, but adding these solves the compilation issues
		// might be microsoft compiler bug
		VulkanRHI(const VulkanRHI&) = delete;
		VulkanRHI& operator=(const VulkanRHI&) = delete;
		VulkanRHI(VulkanRHI&&) noexcept = delete;
		VulkanRHI& operator=(VulkanRHI&&) noexcept = delete;

		virtual void Init(const RendererConfig& config) override;

		virtual void Shutdown() override;

		virtual void BeginFrame(const Ref<Window>& window) override;

		virtual void EndFrame(const Ref<Window>& window) override;

		virtual Ref<CommandContext> GetContext(CommandContextType type) override;

		virtual void ForEachUniqueContext(const std::function<void(const Ref<CommandContext>&)>& func) override;

		virtual Ref<Texture> CreateTexture(const TextureCreateInfo& info) override;

		virtual Ref<TextureView> CreateTextureView(const TextureViewCreateInfo& info, Texture* texture) override;

		virtual Ref<GPUBuffer> CreateGPUBuffer(const GPUBufferCreateInfo& info) override;

		virtual Ref<TracyGPUZone> CreateTracyZone(const Ref<CommandContext>& cmdContext, uint32 line, std::string_view fileName, std::string_view functionName, const String& name) override;

		virtual Ref<SwapChain> CreateSwapChain(void* windowHandle, uint32 width, uint32 height, bool vsync) override;

		virtual Ref<ImGuiBackend> CreateImGuiBackend() override;

		virtual uint64 GetMemoryUsage() override;

		virtual const GPUCapabilities& GetGPUCapabilities() override;

		virtual const FormatSupportProperties& GetFormatSupportProperties(Format format) override;

		// Vulkan functions
	public:
		static VulkanRHI* Get()
		{
			return reinterpret_cast<VulkanRHI*>(RHI::Get());
		}

		VkInstance GetVulkanInstance() const { return m_Instance; }

		VulkanDevice* GetDevice() { return &m_Device; }

		const InstanceExtensionsInfo& GetExtensionsInfo() const { return m_ExtensionsInfo; }

		Ref<VulkanAllocator> GetAllocator() const { return m_Allocator; }

		uint32 GetVulkanVersion() const { return m_VulkanVersion; }

		template <typename FuncT>
		void DeleteFrameResourcesDeferred(FuncT&& func)
		{
			m_ResourceFreeQueues[m_FrameResourcesIndex].Enqueue(std::forward<FuncT>(func));
		}

		uint32 GetFrameResourcesIndex() const
		{
			return m_FrameResourcesIndex;
		}

	private:
		void PostInstanceInit();
		void QueryInstanceLayers();
		void QueryInstanceExtensions();

	private:
		VkInstance m_Instance;
		InstanceExtensionsInfo m_ExtensionsInfo;
		VulkanDevice m_Device;
		Ref<VulkanAllocator> m_Allocator;

		uint32 m_FrameResourcesIndex = 0;
		FramesInFlightArray<CommandStream> m_ResourceFreeQueues;

		uint32 m_VulkanVersion;
		std::vector<const char*> m_InstanceLayers;
		std::vector<const char*> m_InstanceExtensions;
	};
}
