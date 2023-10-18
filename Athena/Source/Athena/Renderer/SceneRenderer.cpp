#include "SceneRenderer.h"

#include "Athena/Math/Projections.h"
#include "Athena/Math/Transforms.h"

#include "Athena/Renderer/Renderer.h"
#include "Athena/Renderer/RenderList.h"
#include "Athena/Renderer/Shader.h"
#include "Athena/Renderer/Image.h"

// TEMPORARY
#include "Athena/Core/Application.h"
#include "Athena/Renderer/CommandBuffer.h"
#include "Athena/Platform/Vulkan/VulkanSwapChain.h"
#include "Athena/Platform/Vulkan/VulkanShader.h"
#include "Athena/Platform/Vulkan/VulkanUtils.h"
#include "Athena/Platform/Vulkan/VulkanCommandBuffer.h"
#include "Athena/Platform/Vulkan/VulkanImage.h"
#include <vulkan/vulkan.h>



namespace Athena
{
	struct CameraUBO
	{
		Matrix4 ViewMatrix;
		Matrix4 ProjectionMatrix;
	};

	struct VulkanUBO
	{
		VkBuffer Buffer;
		VkDeviceMemory BufferMemory;
	};

	static Ref<Shader> s_Shader;

	static VkBuffer s_VertexBuffer;
	static VkDeviceMemory s_VertexBufferMemory;
	static VkBuffer s_IndexBuffer;
	static VkDeviceMemory s_IndexBufferMemory;
	static std::vector<VulkanUBO> s_UniformBuffers;
	static CameraUBO s_CameraUBO;

	static VkDescriptorPool s_DescriptorPool;
	static VkDescriptorSetLayout s_DescriptorSetLayout;
	static std::vector<VkDescriptorSet> s_DescriptorSets;

	static VkRenderPass s_RenderPass;
	static VkPipelineLayout s_PipelineLayout;
	static VkPipeline s_Pipeline;

	static std::vector<Ref<Image>> s_ImageAttachments;
	static std::vector<VkFramebuffer> s_Framebuffers;

	namespace Utils
	{
		void CreateVulkanBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer* buffer, VkDeviceMemory* bufferMemory)
		{
			VkDevice logicalDevice = VulkanContext::GetLogicalDevice();

			VkBufferCreateInfo bufferInfo{};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.size = size;
			bufferInfo.usage = usage;
			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			VK_CHECK(vkCreateBuffer(logicalDevice, &bufferInfo, VulkanContext::GetAllocator(), buffer));

			VkMemoryRequirements memRequirements;
			vkGetBufferMemoryRequirements(logicalDevice, *buffer, &memRequirements);

			VkMemoryAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = memRequirements.size;
			allocInfo.memoryTypeIndex = GetVulkanMemoryType(memRequirements.memoryTypeBits, properties);

			VK_CHECK(vkAllocateMemory(logicalDevice, &allocInfo, VulkanContext::GetAllocator(), bufferMemory));

			vkBindBufferMemory(logicalDevice, *buffer, *bufferMemory, 0);
		}

		void CopyVulkanBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
		{
			Ref<CommandBuffer> commandBuffer = CommandBuffer::Create(CommandBufferUsage::IMMEDIATE);
			commandBuffer->Begin();
			{
				VkCommandBuffer vkCmdBuf = commandBuffer.As<VulkanCommandBuffer>()->GetVulkanCommandBuffer();

				VkBufferCopy copyRegion{};
				copyRegion.srcOffset = 0;
				copyRegion.dstOffset = 0;
				copyRegion.size = size;
				vkCmdCopyBuffer(vkCmdBuf, srcBuffer, dstBuffer, 1, &copyRegion);
			}
			commandBuffer->End();
			commandBuffer->Flush();
		}
	}


	Ref<SceneRenderer> SceneRenderer::Create()
	{
		Ref<SceneRenderer> renderer = Ref<SceneRenderer>::Create();
		renderer->Init();

		return renderer;
	}

	void SceneRenderer::Init()
	{
		m_Data = new SceneRendererData();

		s_Shader = Shader::Create(Renderer::GetShaderPackDirectory() / "Vulkan/Test.hlsl");

	
		// Create Vertex Buffer and Index Buffer
		VkVertexInputBindingDescription bindingDescription = {};
		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions = {};
		{
			struct TVertex
			{
				Vector2 Position;
				LinearColor Color;
			};

			const TVertex vertices[] = {
				{ { -0.5f, -0.5f }, { 1.f, 0.f, 0.f, 1.f } },
				{ {  0.5f, -0.5f }, { 0.f, 1.f, 0.f, 0.f } },
				{ {  0.5f,  0.5f }, { 0.f, 1.f, 0.f, 1.f } },
				{ { -0.5f,  0.5f }, { 1.f, 1.f, 1.f, 1.f } }
			};

			const uint32 indices[] = {
				0, 1, 2, 2, 3, 0
			};

			
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(TVertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;


			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(TVertex, Position); // 0

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(TVertex, Color);  // 8

			VkDevice logicalDevice = VulkanContext::GetDevice()->GetLogicalDevice();

			VkBuffer stagingBuffer;
			VkDeviceMemory stagingBufferMemory;

			// Vertex buffer
			{
				VkDeviceSize bufferSize = sizeof(vertices);

				Utils::CreateVulkanBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);

				void* data;
				vkMapMemory(logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
				memcpy(data, vertices, bufferSize);
				vkUnmapMemory(logicalDevice, stagingBufferMemory);

				Utils::CreateVulkanBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &s_VertexBuffer, &s_VertexBufferMemory);

				Utils::CopyVulkanBuffer(stagingBuffer, s_VertexBuffer, bufferSize);

				vkDestroyBuffer(logicalDevice, stagingBuffer, VulkanContext::GetAllocator());
				vkFreeMemory(logicalDevice, stagingBufferMemory, VulkanContext::GetAllocator());
			}

			// Index buffer
			{
				VkDeviceSize bufferSize = sizeof(indices);

				Utils::CreateVulkanBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);

				void* data;
				vkMapMemory(logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
				memcpy(data, indices, bufferSize);
				vkUnmapMemory(logicalDevice, stagingBufferMemory);

				Utils::CreateVulkanBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &s_IndexBuffer, &s_IndexBufferMemory);

				Utils::CopyVulkanBuffer(stagingBuffer, s_IndexBuffer, bufferSize);

				vkDestroyBuffer(logicalDevice, stagingBuffer, VulkanContext::GetAllocator());
				vkFreeMemory(logicalDevice, stagingBufferMemory, VulkanContext::GetAllocator());
			}
		}


		// Descriptor Sets
		{
			VkDescriptorSetLayoutBinding uboLayoutBinding = {};
			uboLayoutBinding.binding = 0;
			uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			uboLayoutBinding.descriptorCount = 1;
			uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			uboLayoutBinding.pImmutableSamplers = nullptr;

			VkDescriptorSetLayoutCreateInfo layoutInfo = {};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.bindingCount = 1;
			layoutInfo.pBindings = &uboLayoutBinding;

			VK_CHECK(vkCreateDescriptorSetLayout(VulkanContext::GetLogicalDevice(), &layoutInfo, VulkanContext::GetAllocator(), &s_DescriptorSetLayout));

			VkDeviceSize bufferSize = sizeof(CameraUBO);
			s_UniformBuffers.resize(Renderer::GetFramesInFlight());

			for (auto& ubo : s_UniformBuffers)
			{
				Utils::CreateVulkanBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &ubo.Buffer, &ubo.BufferMemory);

			}

			VkDescriptorPoolSize poolSize = {};
			poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			poolSize.descriptorCount = Renderer::GetFramesInFlight();

			VkDescriptorPoolCreateInfo poolInfo = {};
			poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			poolInfo.poolSizeCount = 1;
			poolInfo.pPoolSizes = &poolSize;
			poolInfo.maxSets = Renderer::GetFramesInFlight();

			VK_CHECK(vkCreateDescriptorPool(VulkanContext::GetLogicalDevice(), &poolInfo, VulkanContext::GetAllocator(), &s_DescriptorPool));

			std::vector<VkDescriptorSetLayout> layouts(Renderer::GetFramesInFlight(), s_DescriptorSetLayout);

			VkDescriptorSetAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = s_DescriptorPool;
			allocInfo.descriptorSetCount = Renderer::GetFramesInFlight();
			allocInfo.pSetLayouts = layouts.data();

			s_DescriptorSets.resize(Renderer::GetFramesInFlight());
			VK_CHECK(vkAllocateDescriptorSets(VulkanContext::GetLogicalDevice(), &allocInfo, s_DescriptorSets.data()));

			for (uint32 i = 0; i < Renderer::GetFramesInFlight(); ++i)
			{
				VkDescriptorBufferInfo bufferInfo = {};
				bufferInfo.buffer = s_UniformBuffers[i].Buffer;
				bufferInfo.offset = 0;
				bufferInfo.range = sizeof(CameraUBO);

				VkWriteDescriptorSet descriptorWrite{};
				descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrite.dstSet = s_DescriptorSets[i];
				descriptorWrite.dstBinding = 0;
				descriptorWrite.dstArrayElement = 0;
				descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				descriptorWrite.descriptorCount = 1;
				descriptorWrite.pBufferInfo = &bufferInfo;
				descriptorWrite.pImageInfo = nullptr;
				descriptorWrite.pTexelBufferView = nullptr;

				vkUpdateDescriptorSets(VulkanContext::GetLogicalDevice(), 1, &descriptorWrite, 0, nullptr);
			}

		}

		// Create RenderPass
		{
			VkAttachmentDescription colorAttachment = {};
			colorAttachment.format = VK_FORMAT_R8G8B8A8_UNORM;
			colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;


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

			VK_CHECK(vkCreateRenderPass(VulkanContext::GetLogicalDevice(), &renderPassInfo, VulkanContext::GetAllocator(), &s_RenderPass));
		}

		// Create Attachemnts and Framebuffers
		{
			uint32 width = 1600;
			uint32 height = 900;

			ImageCreateInfo info = {};
			info.Format = ImageFormat::RGBA8;
			info.Width = width;
			info.Height = height;
			info.Usage = ImageUsage::ATTACHMENT;

			s_ImageAttachments.resize(Renderer::GetFramesInFlight());
			for (uint32 i = 0; i < s_ImageAttachments.size(); ++i)
				s_ImageAttachments[i] = Image::Create(info);


			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = s_RenderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.width = width;
			framebufferInfo.height = height;
			framebufferInfo.layers = 1;

			s_Framebuffers.resize(s_ImageAttachments.size());
			for (size_t i = 0; i < s_ImageAttachments.size(); i++)
			{
				VkImageView attachment = s_ImageAttachments[i].As<VulkanImage>()->GetVulkanImageView();
				framebufferInfo.pAttachments = &attachment;

				VK_CHECK(vkCreateFramebuffer(VulkanContext::GetLogicalDevice(), &framebufferInfo, VulkanContext::GetAllocator(), &s_Framebuffers[i]));
			}
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
			rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
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
			colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
			colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; 
			colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
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
			pipelineLayoutInfo.setLayoutCount = 1;
			pipelineLayoutInfo.pSetLayouts = &s_DescriptorSetLayout;
			pipelineLayoutInfo.pushConstantRangeCount = 0;
			pipelineLayoutInfo.pPushConstantRanges = nullptr;

			VK_CHECK(vkCreatePipelineLayout(VulkanContext::GetLogicalDevice(), &pipelineLayoutInfo, VulkanContext::GetAllocator(), &s_PipelineLayout));

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

			VK_CHECK(vkCreateGraphicsPipelines(VulkanContext::GetLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, VulkanContext::GetAllocator(), &s_Pipeline));
		}
	}

	void SceneRenderer::Shutdown()
	{
		s_Shader.Reset();
		s_ImageAttachments.clear();

		Renderer::SubmitResourceFree([]()
			{
				for (uint32 i = 0; i < Renderer::GetFramesInFlight(); ++i)
				{
					vkDestroyFramebuffer(VulkanContext::GetLogicalDevice(), s_Framebuffers[i], VulkanContext::GetAllocator());

					vkDestroyBuffer(VulkanContext::GetLogicalDevice(), s_UniformBuffers[i].Buffer, VulkanContext::GetAllocator());
					vkFreeMemory(VulkanContext::GetLogicalDevice(), s_UniformBuffers[i].BufferMemory, VulkanContext::GetAllocator());
				}
				
				vkDestroyBuffer(VulkanContext::GetLogicalDevice(), s_VertexBuffer, VulkanContext::GetAllocator());
				vkFreeMemory(VulkanContext::GetLogicalDevice(), s_VertexBufferMemory, VulkanContext::GetAllocator());

				vkDestroyBuffer(VulkanContext::GetLogicalDevice(), s_IndexBuffer, VulkanContext::GetAllocator());
				vkFreeMemory(VulkanContext::GetLogicalDevice(), s_IndexBufferMemory, VulkanContext::GetAllocator());

				vkDestroyDescriptorPool(VulkanContext::GetLogicalDevice(), s_DescriptorPool, VulkanContext::GetAllocator());
				vkDestroyDescriptorSetLayout(VulkanContext::GetLogicalDevice(), s_DescriptorSetLayout, VulkanContext::GetAllocator());

				vkDestroyPipeline(VulkanContext::GetLogicalDevice(), s_Pipeline, VulkanContext::GetAllocator());
				vkDestroyPipelineLayout(VulkanContext::GetLogicalDevice(), s_PipelineLayout, VulkanContext::GetAllocator());
				vkDestroyRenderPass(VulkanContext::GetLogicalDevice(), s_RenderPass, VulkanContext::GetAllocator());
			});

		delete m_Data;
	}

	// TEMPORARY
	void SceneRenderer::Render(const CameraInfo& cameraInfo)
	{
		VkCommandBuffer commandBuffer = VulkanContext::GetActiveCommandBuffer();
		uint32 width = s_ImageAttachments[Renderer::GetCurrentFrameIndex()]->GetWidth();
		uint32 height = s_ImageAttachments[Renderer::GetCurrentFrameIndex()]->GetHeight();

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = s_RenderPass;
		renderPassInfo.framebuffer = s_Framebuffers[Renderer::GetCurrentFrameIndex()];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = { width, height };
		VkClearValue clearColor = { {0.9f, 0.3f, 0.4f, 1.0f} };
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
			vkCmdBindIndexBuffer(commandBuffer, s_IndexBuffer, 0, VK_INDEX_TYPE_UINT32);

			// Update Uniform buffer
			{
				s_CameraUBO.ViewMatrix = cameraInfo.ViewMatrix;
				s_CameraUBO.ProjectionMatrix = cameraInfo.ProjectionMatrix;

				auto& ubo = s_UniformBuffers[Renderer::GetCurrentFrameIndex()];

				void* mappedMemory;
				vkMapMemory(VulkanContext::GetDevice()->GetLogicalDevice(), ubo.BufferMemory, 0, sizeof(s_CameraUBO), 0, &mappedMemory);
				memcpy(mappedMemory, &s_CameraUBO, sizeof(s_CameraUBO));
				vkUnmapMemory(VulkanContext::GetDevice()->GetLogicalDevice(), ubo.BufferMemory);

				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, s_PipelineLayout, 0, 1, 
					&s_DescriptorSets[Renderer::GetCurrentFrameIndex()], 0, 0);
			}

			vkCmdDrawIndexed(commandBuffer, 6, 1, 0, 0, 0);
		}
		vkCmdEndRenderPass(commandBuffer);
	}

	Ref<Image> SceneRenderer::GetFinalImage()
	{
		return s_ImageAttachments[Renderer::GetCurrentFrameIndex()];
	}

	void SceneRenderer::OnViewportResize(uint32 width, uint32 height)
	{
		Renderer::SubmitResourceFree([framebuffers = s_Framebuffers]()
			{
				for (VkFramebuffer framebuffer : framebuffers)
					vkDestroyFramebuffer(VulkanContext::GetDevice()->GetLogicalDevice(), framebuffer, VulkanContext::GetAllocator());
			});

		ImageCreateInfo info = {};
		info.Format = ImageFormat::RGBA8;
		info.Width = width;
		info.Height = height;
		info.Usage = ImageUsage::ATTACHMENT;

		s_ImageAttachments.resize(Renderer::GetFramesInFlight());
		for (uint32 i = 0; i < s_ImageAttachments.size(); ++i)
			s_ImageAttachments[i] = Image::Create(info);


		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = s_RenderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.width = width;
		framebufferInfo.height = height;
		framebufferInfo.layers = 1;

		s_Framebuffers.resize(s_ImageAttachments.size());
		for (size_t i = 0; i < s_ImageAttachments.size(); i++)
		{
			VkImageView attachment = s_ImageAttachments[i].As<VulkanImage>()->GetVulkanImageView();
			framebufferInfo.pAttachments = &attachment;

			VK_CHECK(vkCreateFramebuffer(VulkanContext::GetLogicalDevice(), &framebufferInfo, VulkanContext::GetAllocator(), &s_Framebuffers[i]));
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
