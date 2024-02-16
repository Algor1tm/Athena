#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/RenderPass.h"
#include "Athena/Renderer/Shader.h"
#include "Athena/Renderer/RenderResource.h"
#include "Athena/Renderer/RenderCommandBuffer.h"


namespace Athena
{
	struct ComputePipelineCreateInfo
	{
		String Name;
		Ref<Shader> Shader;
		Vector3i WorkGroupSize;
	};


	class ATHENA_API ComputePipeline : public RefCounted
	{
	public:
		static Ref<ComputePipeline> Create(const ComputePipelineCreateInfo& info);
		virtual ~ComputePipeline() = default;
		
		virtual void Bind(const Ref<RenderCommandBuffer>& commandBuffer) = 0;

		virtual void SetInput(const String& name, const Ref<RenderResource>& resource) = 0;
		virtual void Bake() = 0;

		const ComputePipelineCreateInfo& GetInfo() const { return m_Info; }

	protected:
		ComputePipelineCreateInfo m_Info;
	};
}
