#include "Shader.h"

#include "Athena/Platform/OpenGl/GLShader.h"
#include "Renderer.h"

#include <format>


namespace Athena
{
	static ShaderType ShaderTypeFromString(const String& type)
	{
		if (type == "VERTEX_SHADER") return ShaderType::VERTEX_SHADER;
		if (type == "FRAGMENT_SHADER" || type == "PIXEL_SHADER") return ShaderType::FRAGMENT_SHADER;
		if (type == "GEOMETRY_SHADER") return ShaderType::GEOMETRY_SHADER;
		if (type == "COMPUTE_SHADER") return ShaderType::COMPUTE_SHADER;

		ATN_CORE_ASSERT(false, "Unknown shader type!");
		return ShaderType();
	}


	Ref<Shader> Shader::Create(const FilePath& path)
	{
		FilePath stem = path;

		switch (Renderer::GetAPI())
		{
		case Renderer::API::OpenGL:
			return Ref<GLShader>::Create(stem.concat(L".glsl")); break;
		case Renderer::API::None:
			ATN_CORE_ASSERT(false, "Renderer API None is not supported");
		}

		ATN_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}
	
	Ref<Shader> Shader::Create(const FilePath& path, const String& name)
	{
		FilePath stem = path;

		switch (Renderer::GetAPI())
		{
		case Renderer::API::OpenGL:
			return Ref<GLShader>::Create(stem.concat(L".glsl"), name); break;
		case Renderer::API::None:
			ATN_CORE_ASSERT(false, "Renderer API None is not supported");
		}

		ATN_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	Ref<Shader> Shader::Create(const String& name, const String& vertexSrc, const String& fragmentSrc)
	{
		switch (Renderer::GetAPI())
		{
		case Renderer::API::OpenGL:
			return Ref<GLShader>::Create(name, vertexSrc, fragmentSrc); break;
		case Renderer::API::None:
			ATN_CORE_ASSERT(false, "Renderer API None is not supported");
		}

		ATN_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}
	
	std::unordered_map<ShaderType, String> Shader::PreProcess(const String& source)
	{
		std::unordered_map<ShaderType, String> shaderSources;

		const char* typeToken = "#type";
		uint64 typeTokenLength = strlen(typeToken);
		uint64 pos = source.find(typeToken, 0);
		while (pos != String::npos)
		{
			uint64 eol = source.find_first_of("\r\n", pos);
			ATN_CORE_ASSERT(eol != String::npos, "Syntax Error");
			uint64 begin = pos + typeTokenLength + 1;
			String typeString = source.substr(begin, eol - begin);
			ATN_CORE_ASSERT(typeString == "VERTEX_SHADER" || typeString == "FRAGMENT_SHADER" || 
				typeString == "PIXEL_SHADER" || typeString == "GEOMETRY_SHADER" || typeString == "COMPUTE_SHADER",
				"Invalid Shader Type specifier");
			
			ShaderType type = ShaderTypeFromString(typeString);

			uint64 nextLinePos = source.find_first_not_of("\r,\n", eol);
			pos = source.find(typeToken, nextLinePos);
			shaderSources[type] = source.substr(nextLinePos, pos - (nextLinePos == String::npos ? source.size() - 1 : nextLinePos));
		}

		return shaderSources;
	}

	Ref<IncludeShader> IncludeShader::Create(const FilePath& path)
	{
		FilePath stem = path;

		switch (Renderer::GetAPI())
		{
		case Renderer::API::OpenGL:
			return Ref<GLIncludeShader>::Create(stem.concat(L".glsl")); break;
		case Renderer::API::None:
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

	void ShaderLibrary::AddIncludeShader(const String& name, const Ref<IncludeShader>& shader)
	{
		ATN_CORE_ASSERT(!Exists(name), "Shader already exists!");
		m_IncludeShaders[name] = shader;
	}

	Ref<IncludeShader> ShaderLibrary::LoadIncludeShader(const String& name, const FilePath& path)
	{
		auto shader = IncludeShader::Create(path);
		Add(name, shader);
		return shader;
	}

	Ref<Shader> ShaderLibrary::Load(const String& name, const FilePath& path)
	{
		auto shader = Shader::Create(path, name);
		Add(name, shader);
		return shader;
	}

	Ref<Shader> ShaderLibrary::Get(const String& name)
	{
		ATN_CORE_ASSERT(Exists(name), "Shader not found!");
		return m_Shaders.at(name);
	}

	bool ShaderLibrary::Exists(const String& name)
	{
		return (m_Shaders.find(name) != m_Shaders.end()) || 
			(m_IncludeShaders.find(name) != m_IncludeShaders.end());
	}

	void ShaderLibrary::Reload()
	{
		for (const auto& [key, shader] : m_IncludeShaders)
			shader->Reload();

		for (const auto& [key, shader] : m_Shaders)
			shader->Reload();
	}
}
