#include "atnpch.h"
#include "Shader.h"

#include "Athena/Platform/OpenGl/OpenGLShader.h"
#include "Renderer.h"


namespace Athena
{
	Ref<Shader> Shader::Create(const String& filepath)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			return std::make_shared<OpenGLShader>(filepath); break;
		case RendererAPI::API::Direct3D:
			ATN_CORE_ASSERT(false, "Renderer API Direct3D is not supported"); break;
		case RendererAPI::API::None:
			ATN_CORE_ASSERT(false, "Renderer API None is not supported");
		}

		ATN_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	Ref<Shader> Shader::Create(const String& name, const String& vertexSrc, const String& fragmentSrc)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			return std::make_shared<OpenGLShader>(name, vertexSrc, fragmentSrc); break;
		case RendererAPI::API::Direct3D:
			ATN_CORE_ASSERT(false, "Renderer API Direct3D is not supported"); break;
		case RendererAPI::API::None:
			ATN_CORE_ASSERT(false, "Renderer API None is not supported");
		}

		ATN_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}
	

	void ShaderLibrary::Add(const String& name, const Ref<Shader>& shader)
	{
		ATN_CORE_ASSERT(!Exists(name), "Shader already exists!");
		m_Shaders[name] = shader;
	}

	void ShaderLibrary::Add(const Ref<Shader>& shader)
	{
		const String& name = shader->GetName();
		Add(name, shader);
	}

	Ref<Shader> ShaderLibrary::Load(const String& filepath)
	{
		auto shader = Shader::Create(filepath);
		Add(shader);
		return shader;
	}

	Ref<Shader> ShaderLibrary::Load(const String& name, const String& filepath)
	{
		auto shader = Shader::Create(filepath);
		Add(shader);
		return shader;
	}

	Ref<Shader> ShaderLibrary::Get(const String& name)
	{
		ATN_CORE_ASSERT(Exists(name), "Shader not found!");
		return m_Shaders.at(name);
	}

	bool ShaderLibrary::Exists(const String& name)
	{
		return m_Shaders.find(name) != m_Shaders.end();
	}
}