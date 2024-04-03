#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/RenderPass.h"
#include "Athena/Renderer/Shader.h"
#include "Athena/Renderer/RenderResource.h"
#include "Athena/Renderer/RenderCommandBuffer.h"


namespace Athena
{
	enum class Topology
	{
		TRIANGLE_LIST = 1,
		LINE_LIST
	};

	enum class CullMode
	{
		NONE = 0,
		BACK,
		FRONT
	};

	enum class DepthCompare
	{
		NONE = 0,
		LESS,
		LESS_OR_EQUAL
	};

	struct PipelineCreateInfo
	{
		String Name;
		Ref<RenderPass> RenderPass;
		Ref<Shader> Shader;
		VertexMemoryLayout VertexLayout;
		VertexMemoryLayout InstanceLayout;
		Topology Topology = Topology::TRIANGLE_LIST;
		CullMode CullMode = CullMode::BACK;
		DepthCompare DepthCompare = DepthCompare::NONE;
		bool BlendEnable = true;
	};


	class ATHENA_API Pipeline : public RefCounted
	{
	public:
		static Ref<Pipeline> Create(const PipelineCreateInfo& info);
		virtual ~Pipeline() = default;
		
		virtual void Bind(const Ref<RenderCommandBuffer>& commandBuffer) = 0;
		virtual void SetViewport(uint32 width, uint32 height) = 0;

		virtual void SetLineWidth(const Ref<RenderCommandBuffer>& commandBuffer, float width) = 0;

		virtual void SetInput(const String& name, const Ref<RenderResource>& resource) = 0;
		virtual void Bake() = 0;

		const PipelineCreateInfo& GetInfo() const { return m_Info; }

	protected:
		PipelineCreateInfo m_Info;
	};
}
