#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Color.h"
#include "Athena/Renderer/Texture.h"
#include "Athena/Renderer/RenderCommandBuffer.h"
#include "Athena/Renderer/RenderResource.h"
#include "Athena/Renderer/Shader.h"


namespace Athena
{
	struct ComputePassCreateInfo
	{
		String Name;
		Ref<Shader> Shader;
		LinearColor DebugColor = LinearColor(0.f);
	};

	class ATHENA_API ComputePass: public RefCounted
	{
	public:
		static Ref<ComputePass> Create(const ComputePassCreateInfo& info);
		virtual ~ComputePass() = default;

		virtual void Begin(const Ref<RenderCommandBuffer>& commandBuffer) = 0;
		virtual void End(const Ref<RenderCommandBuffer>& commandBuffer) = 0;

		virtual void SetInput(const String& name, const Ref<RenderResource>& resource) = 0;
		virtual void Bake() = 0;

		void SetOutput(const Ref<Texture2D>& texture)
		{
			m_Outputs.push_back(texture->GetImage());
		}

		void SetOutput(const Ref<TextureCube>& texture)
		{
			m_Outputs.push_back(texture->GetImage());
		}

		void SetOutput(const Ref<Image>& image)
		{
			m_Outputs.push_back(image);
		}

		const ComputePassCreateInfo& GetInfo() const { return m_Info; }

	protected:
		ComputePassCreateInfo m_Info;
		std::vector<Ref<Image>> m_Outputs;
	};
}
