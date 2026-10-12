#pragma once 

#include "Athena/Core/Core.h"
#include "Athena/Renderer/RHI/RHI.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanConfiguration.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanDebug.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanDevice.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	struct OptionalInstanceExtensionsInfo
	{
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

		const OptionalInstanceExtensionsInfo& GetOptionalExtensionsInfo() const { return m_OptionalExtensionsInfo; }

		uint32 GetVulkanVersion() const { return m_VulkanVersion; }


	private:
		void QueryInstanceLayers();
		void QueryInstanceExtensions();

	private:
		VkInstance m_Instance;
		OptionalInstanceExtensionsInfo m_OptionalExtensionsInfo;
		VulkanDevice m_Device;

		uint32 m_VulkanVersion;
		std::vector<const char*> m_InstanceLayers;
		std::vector<const char*> m_InstanceExtensions;
	};


}
