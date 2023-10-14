#include "SceneRenderer.h"

#include "Athena/Math/Projections.h"
#include "Athena/Math/Transforms.h"

#include "Athena/Renderer/Renderer.h"
#include "Athena/Renderer/RenderList.h"
#include "Athena/Renderer/Shader.h"

// TEMPORARY
#include "Athena/Core/Application.h"
#include "Athena/Renderer/CommandBuffer.h"
#include "Athena/Platform/Vulkan/VulkanSwapChain.h"
#include "Athena/Platform/Vulkan/VulkanDebug.h"
#include "Athena/Platform/Vulkan/VulkanRenderer.h"
#include "Athena/Platform/Vulkan/VulkanShader.h"
#include <vulkan/vulkan.h>


namespace Athena
{
	static Ref<Shader> s_Shader;

	static VkBuffer s_VertexBuffer;
	static VkDeviceMemory s_VertexBufferMemory;

	static VkRenderPass s_RenderPass;
	static VkPipelineLayout s_PipelineLayout;
	static VkPipeline s_Pipeline;

	static std::vector<VkFramebuffer> s_Framebuffers;


	Ref<SceneRenderer> SceneRenderer::Create()
	{
		Ref<SceneRenderer> renderer = Ref<SceneRenderer>::Create();
		renderer->Init();

		return renderer;
	}

	void SceneRenderer::Init()
	{
		m_Data = new SceneRendererData();

		s_Shader = Shader::Create(Renderer::GetShaderPackDirectory() / "Vulkan/Test.glsl");

		// Create Vertex Buffer
		VkVertexInputBindingDescription bindingDescription = {};
		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions = {};
		{
			struct TVertex
			{
				Vector2 Position;
				LinearColor Color;
			};

			const std::vector<TVertex> vertices = {
				{ { 0.0f, -0.5f }, { 1.f, 0.f, 0.f, 0.f }},
				{ { 0.5f,  0.5f }, { 0.f, 1.f, 0.f, 0.f }},
				{ { -0.5f, 0.5f }, { 0.f, 0.f, 1.f, 1.f }}
			};

			
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(TVertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;


			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(TVertex, Position);

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(TVertex, Color);

			VkBufferCreateInfo bufferInfo{};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.size = sizeof(TVertex) * vertices.size();
			bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			VK_CHECK(vkCreateBuffer(VulkanContext::GetDevice()->GetLogicalDevice(), &bufferInfo, VulkanContext::GetAllocator(), &s_VertexBuffer));

			VkMemoryRequirements memRequirements;
			vkGetBufferMemoryRequirements(VulkanContext::GetDevice()->GetLogicalDevice(), s_VertexBuffer, &memRequirements);

			auto findMemoryType = [](uint32 typeFilter, VkMemoryPropertyFlags properties) -> uint32
				{
					VkPhysicalDeviceMemoryProperties memProperties;
					vkGetPhysicalDeviceMemoryProperties(VulkanContext::GetDevice()->GetPhysicalDevice(), &memProperties);

					for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) 
					{
						if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) 
						{
							return i;
						}
					}

					ATN_CORE_ASSERT(false);
					return 0;
				};

			VkMemoryAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = memRequirements.size;
			allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			VK_CHECK(vkAllocateMemory(VulkanContext::GetDevice()->GetLogicalDevice(), &allocInfo, VulkanContext::GetAllocator(), &s_VertexBufferMemory));
			vkBindBufferMemory(VulkanContext::GetDevice()->GetLogicalDevice(), s_VertexBuffer, s_VertexBufferMemory, 0);

			void* data;
			vkMapMemory(VulkanContext::GetDevice()->GetLogicalDevice(), s_VertexBufferMemory, 0, bufferInfo.size, 0, &data);
			memcpy(data, vertices.data(), (size_t)bufferInfo.size);
			vkUnmapMemory(VulkanContext::GetDevice()->GetLogicalDevice(), s_VertexBufferMemory);
		}

		// Create RenderPass
		{
			VkAttachmentDescription colorAttachment = {};
			colorAttachment.format = VK_FORMAT_B8G8R8A8_UNORM;
			colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;


			VkAttachmentReference colorAttachmentRef = {};
			colorAttachmentRef.attachment = 0;
			colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &colorAttachmentRef;


			VkRenderPassCreateInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassInfo.attachmentCount = 1;
			renderPassInfo.pAttachments = &colorAttachment;
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;

			VK_CHECK(vkCreateRenderPass(VulkanContext::GetDevice()->GetLogicalDevice(), &renderPassInfo, VulkanContext::GetAllocator(), &s_RenderPass));
		}

		// Create Pipeline
		{
			VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
			vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertexInputInfo.vertexBindingDescriptionCount = 1;
			vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
			vertexInputInfo.vertexAttributeDescriptionCount = attributeDescriptions.size();
			vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();


			VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
			inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			inputAssembly.primitiveRestartEnable = VK_FALSE;


			VkPipelineViewportStateCreateInfo viewportState = {};
			viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportState.viewportCount = 1;
			viewportState.scissorCount = 1;


			VkPipelineRasterizationStateCreateInfo rasterizer = {};
			rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterizer.depthClampEnable = VK_FALSE;
			rasterizer.rasterizerDiscardEnable = VK_FALSE;
			rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
			rasterizer.lineWidth = 1.0f;
			rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
			rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
			rasterizer.depthBiasEnable = VK_FALSE;
			rasterizer.depthBiasConstantFactor = 0.0f;
			rasterizer.depthBiasClamp = 0.0f; 
			rasterizer.depthBiasSlopeFactor = 0.0f; 


			VkPipelineMultisampleStateCreateInfo multisampling = {};
			multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multisampling.sampleShadingEnable = VK_FALSE;
			multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
			multisampling.minSampleShading = 1.0f; 
			multisampling.pSampleMask = nullptr;
			multisampling.alphaToCoverageEnable = VK_FALSE; 
			multisampling.alphaToOneEnable = VK_FALSE;

			VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
			colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			colorBlendAttachment.blendEnable = VK_TRUE;
			colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; 
			colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
			colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; 
			colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; 


			VkPipelineColorBlendStateCreateInfo colorBlending = {};
			colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			colorBlending.logicOpEnable = VK_FALSE;
			colorBlending.logicOp = VK_LOGIC_OP_COPY;
			colorBlending.attachmentCount = 1;
			colorBlending.pAttachments = &colorBlendAttachment;
			colorBlending.blendConstants[0] = 0.0f; 
			colorBlending.blendConstants[1] = 0.0f;
			colorBlending.blendConstants[2] = 0.0f;
			colorBlending.blendConstants[3] = 0.0f;


			std::vector<VkDynamicState> dynamicStates =
			{
				VK_DYNAMIC_STATE_VIEWPORT,
				VK_DYNAMIC_STATE_SCISSOR
			};

			VkPipelineDynamicStateCreateInfo dynamicState = {};
			dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
			dynamicState.pDynamicStates = dynamicStates.data();


			VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
			pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutInfo.setLayoutCount = 0;
			pipelineLayoutInfo.pSetLayouts = nullptr; 
			pipelineLayoutInfo.pushConstantRangeCount = 0;
			pipelineLayoutInfo.pPushConstantRanges = nullptr;

			VK_CHECK(vkCreatePipelineLayout(VulkanContext::GetDevice()->GetLogicalDevice(), &pipelineLayoutInfo, VulkanContext::GetAllocator(), &s_PipelineLayout));

			auto vkShader = s_Shader.As<VulkanShader>();

			VkGraphicsPipelineCreateInfo pipelineInfo = {};
			pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipelineInfo.stageCount = vkShader->GetPipelineStages().size();
			pipelineInfo.pStages = vkShader->GetPipelineStages().data();
			pipelineInfo.pVertexInputState = &vertexInputInfo;
			pipelineInfo.pInputAssemblyState = &inputAssembly;
			pipelineInfo.pViewportState = &viewportState;
			pipelineInfo.pRasterizationState = &rasterizer;
			pipelineInfo.pMultisampleState = &multisampling;
			pipelineInfo.pDepthStencilState = nullptr;
			pipelineInfo.pColorBlendState = &colorBlending;
			pipelineInfo.pDynamicState = &dynamicState;
			pipelineInfo.layout = s_PipelineLayout;
			pipelineInfo.renderPass = s_RenderPass;
			pipelineInfo.subpass = 0;
			pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
			pipelineInfo.basePipelineIndex = -1;

			VK_CHECK(vkCreateGraphicsPipelines(VulkanContext::GetDevice()->GetLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, VulkanContext::GetAllocator(), &s_Pipeline));
		}

		// Create Framebuffers
		{
			s_Framebuffers.resize(Renderer::GetFramesInFlight());
			Ref<VulkanSwapChain> vkSwapChain = Application::Get().GetWindow().GetSwapChain().As<VulkanSwapChain>();;

			for (size_t i = 0; i < vkSwapChain->GetVulkanImageViews().size(); i++) 
			{
				VkImageView attachment = vkSwapChain->GetVulkanImageViews()[i];

				VkFramebufferCreateInfo framebufferInfo{};
				framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				framebufferInfo.renderPass = s_RenderPass;
				framebufferInfo.attachmentCount = 1;
				framebufferInfo.pAttachments = &attachment;
				framebufferInfo.width = Application::Get().GetWindow().GetWidth();
				framebufferInfo.height = Application::Get().GetWindow().GetHeight();
				framebufferInfo.layers = 1;

				VK_CHECK(vkCreateFramebuffer(VulkanContext::GetDevice()->GetLogicalDevice(), &framebufferInfo, VulkanContext::GetAllocator(), &s_Framebuffers[i]));
			}
		}
	}

	void SceneRenderer::Shutdown()
	{
		s_Shader.Reset();

		Renderer::SubmitResourceFree([]()
			{
				for (VkFramebuffer framebuffer : s_Framebuffers)
					vkDestroyFramebuffer(VulkanContext::GetDevice()->GetLogicalDevice(), framebuffer, VulkanContext::GetAllocator());

				vkDestroyBuffer(VulkanContext::GetDevice()->GetLogicalDevice(), s_VertexBuffer, VulkanContext::GetAllocator());
				vkFreeMemory(VulkanContext::GetDevice()->GetLogicalDevice(), s_VertexBufferMemory, VulkanContext::GetAllocator());

				vkDestroyPipeline(VulkanContext::GetDevice()->GetLogicalDevice(), s_Pipeline, VulkanContext::GetAllocator());
				vkDestroyPipelineLayout(VulkanContext::GetDevice()->GetLogicalDevice(), s_PipelineLayout, VulkanContext::GetAllocator());
				vkDestroyRenderPass(VulkanContext::GetDevice()->GetLogicalDevice(), s_RenderPass, VulkanContext::GetAllocator());
			});

		delete m_Data;
	}

	// TEMPORARY
	void SceneRenderer::RenderTest()
	{
		Window& window = Application::Get().GetWindow();
		Ref<SwapChain> swapChain = window.GetSwapChain();

		VkCommandBuffer commandBuffer = (VkCommandBuffer)Renderer::GetCommandQueue()->GetCommandBuffer();
		VkImage swapChainImage = (VkImage)swapChain->GetCurrentImageHandle();
		uint32 width = window.GetWidth();
		uint32 height = window.GetHeight();

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = s_RenderPass;
		renderPassInfo.framebuffer = s_Framebuffers[swapChain->GetCurrentImageIndex()];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = { width, height };
		VkClearValue clearColor = { {0.0f, 0.0f, 0.0f, 1.0f} };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, s_Pipeline);
			
			VkViewport viewport = {};
			viewport.x = 0.0f;
			viewport.y = 0.0f;
			viewport.width = width;
			viewport.height = height;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;
			vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
			
			VkRect2D scissor = {};
			scissor.offset = { 0, 0 };
			scissor.extent = { width, height };
			vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
			
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &s_VertexBuffer, offsets);
			
			vkCmdDraw(commandBuffer, 3, 1, 0, 0);
		}
		vkCmdEndRenderPass(commandBuffer);
	}

	void SceneRenderer::OnViewportResize(uint32 width, uint32 height)
	{
		Renderer::SubmitResourceFree([framebuffers = s_Framebuffers]()
			{
				for (VkFramebuffer framebuffer : framebuffers)
					vkDestroyFramebuffer(VulkanContext::GetDevice()->GetLogicalDevice(), framebuffer, VulkanContext::GetAllocator());
			});

		// Create Framebuffers
		{
			s_Framebuffers.resize(Renderer::GetFramesInFlight());
			Ref<VulkanSwapChain> vkSwapChain = Application::Get().GetWindow().GetSwapChain().As<VulkanSwapChain>();

			for (size_t i = 0; i < vkSwapChain->GetVulkanImageViews().size(); i++)
			{
				VkImageView attachment = vkSwapChain->GetVulkanImageViews()[i];

				VkFramebufferCreateInfo framebufferInfo{};
				framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				framebufferInfo.renderPass = s_RenderPass;
				framebufferInfo.attachmentCount = 1;
				framebufferInfo.pAttachments = &attachment;
				framebufferInfo.width = Application::Get().GetWindow().GetWidth();
				framebufferInfo.height = Application::Get().GetWindow().GetHeight();
				framebufferInfo.layers = 1;

				VK_CHECK(vkCreateFramebuffer(VulkanContext::GetDevice()->GetLogicalDevice(), &framebufferInfo, VulkanContext::GetAllocator(), &s_Framebuffers[i]));
			}
		}
	}

	void SceneRenderer::BeginScene(const CameraInfo& cameraInfo)
	{

	}

	void SceneRenderer::EndScene()
	{

	}

	void SceneRenderer::Submit(const Ref<VertexBuffer>& vertexBuffer, const Ref<Material>& material, const Ref<Animator>& animator, const Matrix4& transform, int32 entityID)
	{
		if (vertexBuffer)
		{
			DrawCallInfo info;
			info.VertexBuffer = vertexBuffer;
			info.Material = material;
			info.Animator = animator;
			info.Transform = transform;
			info.EntityID = entityID;

			//s_Data.MeshList.Push(info);
		}
		else
		{
			ATN_CORE_WARN_TAG("SceneRenderer", "Attempt to submit nullptr vertexBuffer!");
		}
	}

	void SceneRenderer::SubmitLightEnvironment(const LightEnvironment& lightEnv)
	{
		if (lightEnv.DirectionalLights.size() > ShaderDef::MAX_DIRECTIONAL_LIGHT_COUNT)
			ATN_CORE_WARN_TAG("SceneRenderer", "Attempt to submit more than {} DirectionalLights!", ShaderDef::MAX_DIRECTIONAL_LIGHT_COUNT);

		if (lightEnv.PointLights.size() > ShaderDef::MAX_POINT_LIGHT_COUNT)
			ATN_CORE_WARN_TAG("SceneRenderer", "Attempt to submit more than {} PointLights!", ShaderDef::MAX_POINT_LIGHT_COUNT);

		//s_Data.LightDataBuffer.DirectionalLightCount = Math::Min(lightEnv.DirectionalLights.size(), (uint64)ShaderDef::MAX_DIRECTIONAL_LIGHT_COUNT);
		//for (uint32 i = 0; i < s_Data.LightDataBuffer.DirectionalLightCount; ++i)
		//{
		//	s_Data.LightDataBuffer.DirectionalLightBuffer[i] = lightEnv.DirectionalLights[i];
		//}
		//
		//s_Data.LightDataBuffer.PointLightCount = Math::Min(lightEnv.PointLights.size(), (uint64)ShaderDef::MAX_POINT_LIGHT_COUNT);
		//for (uint32 i = 0; i < s_Data.LightDataBuffer.PointLightCount; ++i)
		//{
		//	s_Data.LightDataBuffer.PointLightBuffer[i] = lightEnv.PointLights[i];
		//}
		//
		//s_Data.EnvironmentMap = lightEnv.EnvironmentMap;
		//
		//s_Data.EnvMapDataBuffer.LOD = lightEnv.EnvironmentMapLOD;
		//s_Data.EnvMapDataBuffer.Intensity = lightEnv.EnvironmentMapIntensity;
	}

	SceneRendererSettings& SceneRenderer::GetSettings()
	{
		return m_Data->Settings;
	}
}
