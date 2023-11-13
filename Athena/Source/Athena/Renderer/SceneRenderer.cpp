#include "SceneRenderer.h"

#include "Athena/Math/Projections.h"
#include "Athena/Math/Transforms.h"

#include "Athena/Renderer/Renderer.h"
#include "Athena/Renderer/RenderList.h"
#include "Athena/Renderer/Shader.h"
#include "Athena/Renderer/Texture.h" 
#include "Athena/Renderer/RenderPass.h"


// TEMPORARY
#include "Athena/Core/Application.h"
#include "Athena/Platform/Vulkan/VulkanShader.h"
#include "Athena/Platform/Vulkan/VulkanMaterial.h"
#include "Athena/Platform/Vulkan/VulkanVertexBuffer.h"
#include "Athena/Platform/Vulkan/VulkanUniformBuffer.h"
#include "Athena/Platform/Vulkan/VulkanUtils.h"
#include "Athena/Platform/Vulkan/VulkanTexture2D.h"
#include "Athena/Platform/Vulkan/VulkanRenderPass.h"
#include "Athena/Platform/Vulkan/VulkanFramebuffer.h"
#include <vulkan/vulkan.h>



namespace Athena
{
	struct CameraUBO
	{
		Matrix4 ViewMatrix;
		Matrix4 ProjectionMatrix;
	};

	static Ref<Shader> s_Shader;
	static Ref<Material> s_Material;
	static Ref<VertexBuffer> s_VertexBuffer;

	static CameraUBO s_CameraUBO;
	static Ref<UniformBuffer> s_UniformBuffer;

	static Ref<Framebuffer> s_Framebuffer;
	static Ref<RenderPass> s_RenderPass;

	static VkPipeline s_Pipeline;

	Ref<SceneRenderer> SceneRenderer::Create()
	{
		Ref<SceneRenderer> renderer = Ref<SceneRenderer>::Create();
		renderer->Init();

		return renderer;
	}

	void SceneRenderer::Init()
	{
		m_Data = new SceneRendererData();

		const uint32 maxTimestamps = 16;
		const uint32 maxPipelineQueries = 1;
		m_Data->Profiler = GPUProfiler::Create(maxTimestamps, maxPipelineQueries);

		s_Shader = Shader::Create(Renderer::GetShaderPackDirectory() / "Vulkan/Test.hlsl");

		s_UniformBuffer = UniformBuffer::Create(sizeof(CameraUBO));

		FramebufferCreateInfo fbInfo;
		fbInfo.Attachments = { TextureFormat::RGBA8 };
		fbInfo.Width = 1;
		fbInfo.Height = 1;

		s_Framebuffer = Framebuffer::Create(fbInfo);
		
		RenderPassCreateInfo passInfo;
		passInfo.OutputTarget = s_Framebuffer;
		passInfo.LoadOpClear = true;

		s_RenderPass = RenderPass::Create(passInfo);

		s_Material = Material::Create(s_Shader);
		s_Material->Set("u_CameraData", s_UniformBuffer);


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


			VertexBufferCreateInfo info;
			info.VerticesData = (void*)vertices;
			info.VerticesSize = sizeof(vertices);
			info.IndicesData = (void*)indices;
			info.IndicesCount = std::size(indices);
			info.Usage = VertexBufferUsage::STATIC;

			s_VertexBuffer = VertexBuffer::Create(info);

		}

		Renderer::Submit([]()
		{
			// Create Pipeline
			const VertexBufferLayout& vertexBufferLayout = s_Shader->GetReflectionData().VertexBufferLayout;

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

			auto vkShader = s_Shader.As<VulkanShader>();
			auto vkMaterial = s_Material.As<VulkanMaterial>();

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
			pipelineInfo.layout = vkMaterial->GetPipelineLayout();
			pipelineInfo.renderPass = s_RenderPass.As<VulkanRenderPass>()->GetVulkanRenderPass();
			pipelineInfo.subpass = 0;
			pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
			pipelineInfo.basePipelineIndex = -1;

			VK_CHECK(vkCreateGraphicsPipelines(VulkanContext::GetLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &s_Pipeline));
		});
	}

	void SceneRenderer::Shutdown()
	{
		s_Shader.Release();
		s_VertexBuffer.Release();
		s_UniformBuffer.Release();

		s_Framebuffer.Release();
		s_RenderPass.Release();

		Renderer::SubmitResourceFree([]()
		{
			vkDestroyPipeline(VulkanContext::GetLogicalDevice(), s_Pipeline, nullptr);
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

				s_UniformBuffer->RT_SetData(&s_CameraUBO, sizeof(s_CameraUBO));
			}

			// Geometry Pass
			uint32 width = s_Framebuffer->GetInfo().Width;
			uint32 height = s_Framebuffer->GetInfo().Height;

			VkRenderPassBeginInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = s_RenderPass.As<VulkanRenderPass>()->GetVulkanRenderPass();
			renderPassInfo.framebuffer = s_Framebuffer.As<VulkanFramebuffer>()->GetVulkanFramebuffer();
			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent = { width , height};
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

				s_Material->RT_UpdateForRendering();

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
		return { s_Framebuffer->GetInfo().Width, s_Framebuffer->GetInfo().Height };
	}

	Ref<Texture2D> SceneRenderer::GetFinalImage()
	{
		return s_Framebuffer->GetColorAttachment(0);
	}

	void SceneRenderer::OnViewportResize(uint32 width, uint32 height)
	{
		s_Framebuffer->Resize(width, height);
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
