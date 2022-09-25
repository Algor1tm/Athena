#include "atnpch.h"
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


	Ref<Shader> Shader::Create(const BufferLayout& layout, const Filepath& filepath)
	{
		Filepath withExtension = filepath;

		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			return CreateRef<GLShader>(withExtension.concat(L".glsl")); break;
		case RendererAPI::API::Direct3D:
			return CreateRef<D3D11Shader>(layout, withExtension.concat(L".hlsl")); break;
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
	
	String Shader::ReadFile(const String& filepath)
	{
		String result;
		std::ifstream in(filepath, std::ios::in | std::ios::binary);
		if (in)
		{
			in.seekg(0, std::ios::end);
			result.resize(in.tellg());
			in.seekg(0, std::ios::beg);
			in.read(result.data(), result.size());
			in.close();
		}
		else
		{
			ATN_CORE_ERROR("Could not open file: '{0}'", filepath);
		}
		return result;
	}

	std::unordered_map<ShaderType, String> Shader::PreProcess(const String& source)
	{
		std::unordered_map<ShaderType, String> shaderSources;

		const char* typeToken = "#type";
		SIZE_T typeTokenLength = strlen(typeToken);
		SIZE_T pos = source.find(typeToken, 0);
		while (pos != std::string::npos)
		{
			SIZE_T eol = source.find_first_of("\r\n", pos);
			ATN_CORE_ASSERT(eol != std::string::npos, "Syntax Error");
			SIZE_T begin = pos + typeTokenLength + 1;
			String type = source.substr(begin, eol - begin);
			ATN_CORE_ASSERT(type == "VERTEX_SHADER" || type == "FRAGMENT_SHADER" || type == "PIXEL_SHADER",
				"Invalid Shader Type specifier");

			SIZE_T nextLinePos = source.find_first_not_of("\r,\n", eol);
			pos = source.find(typeToken, nextLinePos);
			shaderSources[ShaderTypeFromString(type)] =
				source.substr(nextLinePos, pos - (nextLinePos == std::string::npos ? source.size() - 1 : nextLinePos));
		}

		return shaderSources;
	}

	void Shader::SetNameFromFilepath(const String& filepath)
	{
		// assets/shaders/Grid.glsl -> m_Name = Grid
		SIZE_T lastSlash = filepath.find_last_of("/\\");
		lastSlash = lastSlash == std::string::npos ? 0 : lastSlash + 1;
		SIZE_T lastDot = filepath.rfind('.');
		SIZE_T count = lastDot == std::string::npos ? filepath.size() - lastSlash : lastDot - lastSlash;
		m_Name = filepath.substr(lastSlash, count);
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

	Ref<Shader> ShaderLibrary::Load(const BufferLayout& layout, const String& filepath)
	{
		auto shader = Shader::Create(layout, filepath);
		Add(shader);
		return shader;
	}

	Ref<Shader> ShaderLibrary::Load(const BufferLayout& layout, const String& name, const String& filepath)
	{
		auto shader = Shader::Create(layout, filepath);
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
