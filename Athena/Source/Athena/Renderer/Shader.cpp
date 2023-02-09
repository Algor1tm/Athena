#include "Shader.h"

#include "Athena/Platform/OpenGl/GLShader.h"
#include "Athena/Platform/Direct3D/D3D11Shader.h"
#include "Renderer.h"


namespace Athena
{
	static ShaderType ShaderTypeFromString(const String& type)
	{
		if (type == "VERTEX_SHADER") return ShaderType::VERTEX_SHADER;
		if (type == "FRAGMENT_SHADER" || type == "PIXEL_SHADER") return ShaderType::FRAGMENT_SHADER;

		ATN_CORE_ASSERT(false, "Unknown shader type!");
		return ShaderType();
	}


	Ref<Shader> Shader::Create(const BufferLayout& layout, const FilePath& path)
	{
		FilePath stem = path;

		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			return CreateRef<GLShader>(stem.concat(L".glsl")); break;
		case RendererAPI::API::Direct3D:
			return CreateRef<D3D11Shader>(layout, stem.concat(L".hlsl")); break;
		case RendererAPI::API::None:
			ATN_CORE_ASSERT(false, "Renderer API None is not supported");
		}

		ATN_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	Ref<Shader> Shader::Create(const BufferLayout& layout, const String& name, const String& vertexSrc, const String& fragmentSrc)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			return CreateRef<GLShader>(name, vertexSrc, fragmentSrc); break;
		case RendererAPI::API::Direct3D:
			return CreateRef<D3D11Shader>(layout, name, vertexSrc, fragmentSrc); break;
		case RendererAPI::API::None:
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
			String type = source.substr(begin, eol - begin);
			ATN_CORE_ASSERT(type == "VERTEX_SHADER" || type == "FRAGMENT_SHADER" || type == "PIXEL_SHADER",
				"Invalid Shader Type specifier");

			uint64 nextLinePos = source.find_first_not_of("\r,\n", eol);
			pos = source.find(typeToken, nextLinePos);
			shaderSources[ShaderTypeFromString(type)] =
				source.substr(nextLinePos, pos - (nextLinePos == String::npos ? source.size() - 1 : nextLinePos));
		}

		return shaderSources;
	}

	Ref<IncludeShader> IncludeShader::Create(const FilePath& path)
	{
		FilePath stem = path;

		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			return CreateRef<GLIncludeShader>(stem.concat(L".glsl")); break;
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

	Ref<Shader> ShaderLibrary::Load(const BufferLayout& layout, const FilePath& path)
	{
		auto shader = Shader::Create(layout, path);
		Add(shader);
		return shader;
	}

	Ref<Shader> ShaderLibrary::Load(const BufferLayout& layout, const String& name, const FilePath& path)
	{
		auto shader = Shader::Create(layout, path);
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
