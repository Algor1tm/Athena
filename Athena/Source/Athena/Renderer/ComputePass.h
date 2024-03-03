#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Color.h"
#include "Athena/Renderer/Texture.h"
#include "Athena/Renderer/RenderCommandBuffer.h"
#include "Athena/Renderer/RenderResource.h"
#include "Athena/Renderer/Shader.h"


namespace Athena
{
	class RenderPass;
	class ComputePass;

	struct ComputePassCreateInfo
	{
		String Name;
		LinearColor DebugColor = LinearColor(0.f);
		Ref<RenderPass> InputRenderPass;
		Ref<ComputePass> InputComputePass;
	};

	class ATHENA_API ComputePass: public RefCounted
	{
	public:
		static Ref<ComputePass> Create(const ComputePassCreateInfo& info);
		virtual ~ComputePass() = default;

		virtual void Begin(const Ref<RenderCommandBuffer>& commandBuffer) = 0;
		virtual void End(const Ref<RenderCommandBuffer>& commandBuffer) = 0;

		virtual void Bake() = 0;

		void SetOutput(const Ref<Texture2D>& texture);
		void SetOutput(const Ref<TextureCube>& texture);

		Ref<Texture2D> GetOutput(const String& name);
		const std::vector<Ref<Texture2D>>& GetAllOutputs() const { return m_Texture2DOutputs; }

		const ComputePassCreateInfo& GetInfo() const { return m_Info; }

	protected:
		ComputePassCreateInfo m_Info;
		std::vector<Ref<Texture2D>> m_Texture2DOutputs;
		std::vector<Ref<TextureCube>> m_TextureCubeOutputs;
	};
}
