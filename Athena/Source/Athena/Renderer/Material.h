#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Renderer/Color.h"
#include "Athena/Renderer/Shader.h"
#include "Athena/Renderer/GPUBuffers.h"


namespace Athena
{
	class ATHENA_API Material : public RefCounted
	{
	public:
		static Ref<Material> Create(const Ref<Shader>& shader);

		virtual void Set(std::string_view name, const Ref<ShaderResource>& resource) = 0;
		virtual void RT_Bind() = 0;

		Ref<Shader> GetShader() { return m_Shader; };

	protected:
		Ref<Shader> m_Shader;
	};
}
