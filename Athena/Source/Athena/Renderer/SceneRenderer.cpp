#include "SceneRenderer.h"

#include "Athena/Math/Projections.h"
#include "Athena/Math/Transforms.h"

#include "Athena/Renderer/Renderer.h"
#include "Athena/Renderer/RenderList.h"
#include "Athena/Renderer/Shader.h"
#include "Athena/Renderer/Texture.h"

// TEMPORARY
#include "Athena/Core/Application.h"
#include "Athena/Platform/Vulkan/VulkanShader.h"
#include "Athena/Platform/Vulkan/VulkanVertexBuffer.h"
#include "Athena/Platform/Vulkan/VulkanUniformBuffer.h"
#include "Athena/Platform/Vulkan/VulkanUtils.h"
#include "Athena/Platform/Vulkan/VulkanTexture2D.h"
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
	static Ref<VertexBuffer> s_VertexBuffer;

	static CameraUBO s_CameraUBO;
	static Ref<UniformBuffer> s_UniformBuffer;


	static VkDescriptorPool s_DescriptorPool;
	static VkDescriptorSetLayout s_DescriptorSetLayout;
	static std::vector<VkDescriptorSet> s_DescriptorSets;

	static VkRenderPass s_RenderPass;
	static VkPipelineLayout s_PipelineLayout;
	static VkPipeline s_Pipeline;

	static std::vector<Ref<Texture2D>> s_Attachments;
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

		const uint32 maxTimestamps = 32;
		const uint32 maxPipelineQueries = 1;
		m_Data->Profiler = GPUProfiler::Create(maxTimestamps, maxPipelineQueries);

		s_Shader = Shader::Create(Renderer::GetShaderPackDirectory() / "Vulkan/Test.hlsl");
		s_UniformBuffer = UniformBuffer::Create(sizeof(CameraUBO));

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

		VertexBufferLayout layout =
		{
			{ ShaderDataType::Float2, "a_Position"  },
			{ ShaderDataType::Float4, "a_Color"     },
		};

		VertexBufferCreateInfo info;
		info.VerticesData = (void*)vertices;
		info.VerticesSize = sizeof(vertices);
		info.IndicesData = (void*)indices;
		info.IndicesCount = std::size(indices);
		info.Usage = VertexBufferUsage::STATIC;

		s_VertexBuffer = VertexBuffer::Create(info);

		Renderer::Submit([vertexBufferLayout = layout]()
		{
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
					bufferInfo.buffer = s_UniformBuffer.As<VulkanUniformBuffer>()->GetVulkanBuffer(i);
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

				// TODO: if attachment is SwapChain target set layout to VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
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

			// Create Pipeline
			{
				VkVertexInputBindingDescription bindingDescription = {};
				bindingDescription.binding = 0;
				bindingDescription.stride = vertexBufferLayout.GetStride();
				bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

				std::vector<VkVertexInputAttributeDescription> attributeDescriptions(vertexBufferLayout.GetElementsNum());
				for (uint32 i = 0; i < vertexBufferLayout.GetElementsNum(); ++i)
				{
					attributeDescriptions[i].binding = 0;
					attributeDescriptions[i].location = i;
					attributeDescriptions[i].format = 
						VulkanUtils::GetFormat(vertexBufferLayout.GetElements()[i].Type, vertexBufferLayout.GetElements()[i].Normalized);
					attributeDescriptions[i].offset = vertexBufferLayout.GetElements()[i].Offset;
				}

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
				dynamicState.dynamicStateCount = dynamicStates.size();
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
		});


		// Create Attachemnts and Framebuffers
		{
			uint32 width = 1600;
			uint32 height = 900;

			TextureCreateInfo info = {};
			info.Width = width;
			info.Height = height;
			info.GenerateMipMap = false;
			info.sRGB = false;
			info.Format = TextureFormat::RGBA8;
			info.Usage = TextureUsage::ATTACHMENT;
			info.GenerateSampler = true;
			info.SamplerInfo.MinFilter = TextureFilter::LINEAR;
			info.SamplerInfo.MagFilter = TextureFilter::LINEAR;
			info.SamplerInfo.Wrap = TextureWrap::REPEAT;

			s_Attachments.resize(Renderer::GetFramesInFlight());
			for (uint32 i = 0; i < s_Attachments.size(); ++i)
				s_Attachments[i] = Texture2D::Create(info);
		}

		Renderer::Submit([]()
		{
			uint32 width = 1600;
			uint32 height = 900;

			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = s_RenderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.width = width;
			framebufferInfo.height = height;
			framebufferInfo.layers = 1;

			s_Framebuffers.resize(s_Attachments.size());
			for (size_t i = 0; i < s_Attachments.size(); i++)
			{
				VkImageView attachment = s_Attachments[i].As<VulkanTexture2D>()->GetVulkanImageView();
				framebufferInfo.pAttachments = &attachment;

				VK_CHECK(vkCreateFramebuffer(VulkanContext::GetLogicalDevice(), &framebufferInfo, VulkanContext::GetAllocator(), &s_Framebuffers[i]));
			}
		});
	}

	void SceneRenderer::Shutdown()
	{
		s_Shader.Release();
		s_VertexBuffer.Release();
		s_Attachments.clear();

		Renderer::SubmitResourceFree([]()
		{
			for (uint32 i = 0; i < Renderer::GetFramesInFlight(); ++i)
			{
				vkDestroyFramebuffer(VulkanContext::GetLogicalDevice(), s_Framebuffers[i], VulkanContext::GetAllocator());
			}
				
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
		m_Data->Profiler->Reset();

		m_Data->Profiler->BeginPipelineStatsQuery();
		m_Data->Profiler->BeginTimeQuery();

		Renderer::Submit([this, cameraInfo = cameraInfo]()
		{
			VkCommandBuffer commandBuffer = VulkanContext::GetActiveCommandBuffer();

			// Update Uniform buffer
			{
				s_CameraUBO.ViewMatrix = cameraInfo.ViewMatrix;
				s_CameraUBO.ProjectionMatrix = cameraInfo.ProjectionMatrix;

				s_UniformBuffer->SetData(&s_CameraUBO, sizeof(s_CameraUBO));
			}

			// Geometry Pass
			uint32 width = s_Attachments[Renderer::GetCurrentFrameIndex()]->GetInfo().Width;
			uint32 height = s_Attachments[Renderer::GetCurrentFrameIndex()]->GetInfo().Height;

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

				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, s_PipelineLayout, 0, 1,
					&s_DescriptorSets[Renderer::GetCurrentFrameIndex()], 0, 0);

				Ref<VulkanVertexBuffer> vkVertexBuffer = s_VertexBuffer.As<VulkanVertexBuffer>();
				VkBuffer vertexBuffer = vkVertexBuffer->GetVulkanVertexBuffer();

				VkDeviceSize offsets[] = { 0 };
				vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);
				vkCmdBindIndexBuffer(commandBuffer, vkVertexBuffer->GetVulkanIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

				vkCmdDrawIndexed(commandBuffer, s_VertexBuffer->GetIndexCount(), 1, 0, 0, 0);
			}
			vkCmdEndRenderPass(commandBuffer);
		});

		m_Data->Statistics.GeometryPass = m_Data->Profiler->EndTimeQuery();
		m_Data->Statistics.PipelineStats = m_Data->Profiler->EndPipelineStatsQuery();
	}

	Vector2u SceneRenderer::GetViewportSize()
	{
		return { s_Attachments[0]->GetInfo().Width, s_Attachments[0]->GetInfo().Height };
	}

	Ref<Texture2D> SceneRenderer::GetFinalImage()
	{
		return s_Attachments[Renderer::GetCurrentFrameIndex()];
	}

	void SceneRenderer::OnViewportResize(uint32 width, uint32 height)
	{
		Renderer::SubmitResourceFree([framebuffers = s_Framebuffers]()
			{
				for (VkFramebuffer framebuffer : framebuffers)
					vkDestroyFramebuffer(VulkanContext::GetDevice()->GetLogicalDevice(), framebuffer, VulkanContext::GetAllocator());
			});


		TextureCreateInfo info = {};
		info.Width = width;
		info.Height = height;
		info.GenerateMipMap = false;
		info.sRGB = false;
		info.Format = TextureFormat::RGBA8;
		info.Usage = TextureUsage::ATTACHMENT;
		info.GenerateSampler = true;
		info.SamplerInfo.MinFilter = TextureFilter::LINEAR;
		info.SamplerInfo.MagFilter = TextureFilter::LINEAR;
		info.SamplerInfo.Wrap = TextureWrap::REPEAT;

		s_Attachments.resize(Renderer::GetFramesInFlight());
		for (uint32 i = 0; i < s_Attachments.size(); ++i)
			s_Attachments[i] = Texture2D::Create(info);

		Renderer::Submit([this, width = width, height = height]()
		{
			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = s_RenderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.width = width;
			framebufferInfo.height = height;
			framebufferInfo.layers = 1;

			s_Framebuffers.resize(s_Attachments.size());
			for (size_t i = 0; i < s_Attachments.size(); i++)
			{
				VkImageView attachment = s_Attachments[i].As<VulkanTexture2D>()->GetVulkanImageView();
				framebufferInfo.pAttachments = &attachment;

				VK_CHECK(vkCreateFramebuffer(VulkanContext::GetLogicalDevice(), &framebufferInfo, VulkanContext::GetAllocator(), &s_Framebuffers[i]));
			}
		});
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
}
