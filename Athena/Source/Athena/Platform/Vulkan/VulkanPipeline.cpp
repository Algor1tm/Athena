#include "VulkanPipeline.h"

#include "Athena/Platform/Vulkan/VulkanUtils.h"
#include "Athena/Platform/Vulkan/VulkanRenderPass.h"
#include "Athena/Platform/Vulkan/VulkanShader.h"
#include "Athena/Platform/Vulkan/VulkanRenderCommandBuffer.h"


namespace Athena
{
	namespace Vulkan
	{
		static VkPrimitiveTopology GetTopology(Topology topology)
		{
			switch (topology)
			{
			case Topology::TRIANGLE_LIST: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			case Topology::LINE_LIST: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
			}

			ATN_CORE_ASSERT(false);
			return (VkPrimitiveTopology)0;
		}

		static VkCullModeFlags GetCullMode(CullMode cullMode)
		{
			switch (cullMode)
			{
			case CullMode::NONE: return VK_CULL_MODE_NONE;
			case CullMode::BACK: return VK_CULL_MODE_BACK_BIT;
			case CullMode::FRONT: return VK_CULL_MODE_FRONT_BIT;
			}

			ATN_CORE_ASSERT(false);
			return (VkCullModeFlags)0;
		}

		static VkCompareOp GetDepthCompare(DepthCompare depthCompare)
		{
			switch (depthCompare)
			{
			case DepthCompare::NONE: return VK_COMPARE_OP_NEVER;
			case DepthCompare::LESS: return VK_COMPARE_OP_LESS;
			case DepthCompare::LESS_OR_EQUAL: return VK_COMPARE_OP_LESS_OR_EQUAL;
			}

			ATN_CORE_ASSERT(false);
			return (VkCompareOp)0;
		}
	}

	VulkanPipeline::VulkanPipeline(const PipelineCreateInfo& info)
	{
		m_Info = info;

		DescriptorSetManagerCreateInfo setManagerInfo;
		setManagerInfo.Name = info.Name;
		setManagerInfo.Shader = m_Info.Shader;
		setManagerInfo.FirstSet = 1;
		setManagerInfo.LastSet = 4;
		m_DescriptorSetManager = Ref<DescriptorSetManager>::Create(setManagerInfo);

		CreatePipeline(info.RenderPass->GetInfo().Width, info.RenderPass->GetInfo().Height);
	}

	VulkanPipeline::~VulkanPipeline()
	{
		CleanUp();
	}

	void VulkanPipeline::Bind(const Ref<RenderCommandBuffer>& commandBuffer)
	{
		Renderer::Submit([this, commandBuffer]()
		{
			VkCommandBuffer vkcmdBuffer = commandBuffer.As<VulkanRenderCommandBuffer>()->GetVulkanCommandBuffer();
			vkCmdBindPipeline(vkcmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_VulkanPipeline);

			if (m_Info.Topology == Topology::LINE_LIST)
				vkCmdSetLineWidth(vkcmdBuffer, m_Info.LineWidth);

			m_DescriptorSetManager->RT_InvalidateAndUpdate();
			m_DescriptorSetManager->RT_BindDescriptorSets(vkcmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS);
		});
	}

	void VulkanPipeline::SetViewport(uint32 width, uint32 height)
	{
		CleanUp();
		CreatePipeline(width, height);
	}

	void VulkanPipeline::CleanUp()
	{
		Renderer::SubmitResourceFree([pipeline = m_VulkanPipeline]()
		{
			vkDestroyPipeline(VulkanContext::GetLogicalDevice(), pipeline, nullptr);
		});
	}

	void VulkanPipeline::SetInput(const String& name, const Ref<RenderResource>& resource)
	{
		m_DescriptorSetManager->Set(name, resource);
	}

	void VulkanPipeline::Bake()
	{
		m_DescriptorSetManager->Bake();
	}

	void VulkanPipeline::CreatePipeline(uint32 width, uint32 height)
	{
		Renderer::Submit([this, width, height]()
		{
			const VertexBufferLayout& vertexBufferLayout = m_Info.Shader->GetMetaData().VertexBufferLayout;

			VkVertexInputBindingDescription bindingDescription = {};
			bindingDescription.binding = 0;
			bindingDescription.stride = vertexBufferLayout.GetStride();
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			std::vector<VkVertexInputAttributeDescription> attributeDescriptions(vertexBufferLayout.GetElementsNum());
			for (uint32 i = 0; i < vertexBufferLayout.GetElementsNum(); ++i)
			{
				const auto& elem = vertexBufferLayout.GetElements()[i];

				attributeDescriptions[i].binding = 0;
				attributeDescriptions[i].location = elem.Location;
				attributeDescriptions[i].format = Vulkan::GetFormat(elem.Type, elem.Normalized);
				attributeDescriptions[i].offset = elem.Offset;
			}

			VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
			vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertexInputInfo.vertexBindingDescriptionCount = 1;
			vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
			vertexInputInfo.vertexAttributeDescriptionCount = attributeDescriptions.size();
			vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();


			VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
			inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssembly.topology = Vulkan::GetTopology(m_Info.Topology);
			inputAssembly.primitiveRestartEnable = VK_FALSE;

			VkPipelineViewportStateCreateInfo viewportState = {};
			viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;

			VkViewport viewport = {};
			viewport.x = 0.0f;
			viewport.y = 0.0f;
			viewport.width = width;
			viewport.height = height;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;
			viewportState.pViewports = &viewport;
			viewportState.viewportCount = 1;

			VkRect2D scissor = {};
			scissor.offset = { 0, 0 };
			scissor.extent = { width, height };
			viewportState.pScissors = &scissor;
			viewportState.scissorCount = 1;

			VkPipelineRasterizationStateCreateInfo rasterizer = {};
			rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterizer.depthClampEnable = VK_FALSE;
			rasterizer.rasterizerDiscardEnable = VK_FALSE;
			rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
			rasterizer.lineWidth = m_Info.LineWidth;
			rasterizer.cullMode = Vulkan::GetCullMode(m_Info.CullMode);
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

			bool enableDepthTest = m_Info.DepthCompare != DepthCompare::NONE;
			VkPipelineDepthStencilStateCreateInfo depthStencil = {};
			depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			depthStencil.depthTestEnable = enableDepthTest;
			depthStencil.depthWriteEnable = enableDepthTest;
			depthStencil.depthCompareOp = Vulkan::GetDepthCompare(m_Info.DepthCompare);
			depthStencil.depthBoundsTestEnable = VK_FALSE;
			depthStencil.stencilTestEnable = enableDepthTest;
			depthStencil.back.compareOp = depthStencil.depthCompareOp;
			depthStencil.back.failOp = VK_STENCIL_OP_KEEP;
			depthStencil.back.depthFailOp = VK_STENCIL_OP_KEEP;
			depthStencil.back.passOp = VK_STENCIL_OP_REPLACE;
			depthStencil.back.compareMask = 0xff;
			depthStencil.back.writeMask = 0xff;
			depthStencil.back.reference = 1;
			depthStencil.front = depthStencil.back;


			std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments(m_Info.RenderPass->GetColorAttachmentCount());
			for (auto& blendAttachment : colorBlendAttachments)
			{
				blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
				blendAttachment.blendEnable = m_Info.BlendEnable;
				blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
				blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
				blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
				blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
				blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
				blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
			}

			VkPipelineColorBlendStateCreateInfo colorBlending = {};
			colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			colorBlending.logicOpEnable = VK_FALSE;
			colorBlending.logicOp = VK_LOGIC_OP_COPY;
			colorBlending.attachmentCount = colorBlendAttachments.size();
			colorBlending.pAttachments = colorBlendAttachments.data();
			colorBlending.blendConstants[0] = 0.0f;
			colorBlending.blendConstants[1] = 0.0f;
			colorBlending.blendConstants[2] = 0.0f;
			colorBlending.blendConstants[3] = 0.0f;

			auto vkShader = m_Info.Shader.As<VulkanShader>();

			std::vector<VkDynamicState> dynamicStates;
			if (m_Info.Topology == Topology::LINE_LIST)
				dynamicStates.push_back(VK_DYNAMIC_STATE_LINE_WIDTH);

			VkPipelineDynamicStateCreateInfo dynamicState = {};
			dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicState.flags = 0;
			dynamicState.dynamicStateCount = dynamicStates.size();
			dynamicState.pDynamicStates = dynamicStates.data();

			VkGraphicsPipelineCreateInfo pipelineInfo = {};
			pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipelineInfo.stageCount = vkShader->GetPipelineStages().size();
			pipelineInfo.pStages = vkShader->GetPipelineStages().data();
			pipelineInfo.pVertexInputState = &vertexInputInfo;
			pipelineInfo.pInputAssemblyState = &inputAssembly;
			pipelineInfo.pViewportState = &viewportState;
			pipelineInfo.pRasterizationState = &rasterizer;
			pipelineInfo.pMultisampleState = &multisampling;
			pipelineInfo.pDepthStencilState = &depthStencil;
			pipelineInfo.pColorBlendState = &colorBlending;
			pipelineInfo.pDynamicState = &dynamicState;
			pipelineInfo.layout = vkShader->GetPipelineLayout();
			pipelineInfo.renderPass = m_Info.RenderPass.As<VulkanRenderPass>()->GetVulkanRenderPass();
			pipelineInfo.subpass = 0;
			pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
			pipelineInfo.basePipelineIndex = -1;

			VK_CHECK(vkCreateGraphicsPipelines(VulkanContext::GetLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_VulkanPipeline));
			Vulkan::SetObjectDebugName(m_VulkanPipeline, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT, m_Info.Name);
		});
	}
}
