#include "Shader.h"

#include "Athena/Renderer/Renderer.h"

#include "Athena/Platform/Vulkan/VulkanShader.h"

#include <format>


namespace Athena
{
	static ShaderType ShaderTypeFromString(const String& type)
	{
		if (type == "VERTEX_STAGE") return ShaderType::VERTEX_SHADER;
		if (type == "FRAGMENT_STAGE" || type == "PIXEL_STAGE") return ShaderType::FRAGMENT_SHADER;
		if (type == "GEOMETRY_STAGE") return ShaderType::GEOMETRY_SHADER;
		if (type == "COMPUTE_STAGE") return ShaderType::COMPUTE_SHADER;

		ATN_CORE_ASSERT(false, "Unknown shader type!");
		return ShaderType();
	}


	Ref<Shader> Shader::Create(const FilePath& path)
	{
		switch (Renderer::GetAPI())
		{
		case Renderer::API::Vulkan: return Ref<VulkanShader>::Create(path);
		case Renderer::API::None: return nullptr;
		}

		return nullptr;
	}
	
	Ref<Shader> Shader::Create(const FilePath& path, const String& name)
	{
		switch (Renderer::GetAPI())
		{
		case Renderer::API::Vulkan: return Ref<VulkanShader>::Create(path, name);
		case Renderer::API::None: return nullptr;
		}

		return nullptr;
	}

	std::unordered_map<ShaderType, String> Shader::PreProcess(const String& source)
	{
		std::unordered_map<ShaderType, String> shaderSources;

		const char* stageToken = "#pragma";
		uint64 stageTokenLength = strlen(stageToken);
		uint64 pos = source.find(stageToken, 0);
		while (pos != String::npos)
		{
			uint64 eol = source.find_first_of("\r\n", pos);
			ATN_CORE_ASSERT(eol != String::npos, "Syntax Error");
			uint64 begin = pos + stageTokenLength + 1;
			String typeString = source.substr(begin, eol - begin);

			ShaderType type = ShaderTypeFromString(typeString);

			uint64 nextLinePos = source.find_first_not_of("\r,\n", eol);
			pos = source.find(stageToken, nextLinePos);
			shaderSources[type] = source.substr(nextLinePos, pos - (nextLinePos == String::npos ? source.size() - 1 : nextLinePos));
		}

		return shaderSources;
	}


	void ShaderLibrary::Add(const String& name, const Ref<Shader>& shader)
	{
		ATN_CORE_ASSERT(!Exists(name), "Shader already exists!");
		m_Shaders[name] = shader;
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
		return (m_Shaders.find(name) != m_Shaders.end());
	}

	void ShaderLibrary::Reload()
	{
		for (const auto& [key, shader] : m_Shaders)
			shader->Reload();
	}
}
