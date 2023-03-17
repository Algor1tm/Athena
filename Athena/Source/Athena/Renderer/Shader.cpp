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
		case RendererAPI::API::OpenGL:
			return CreateRef<GLShader>(stem.concat(L".glsl")); break;
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
			return CreateRef<GLShader>(name, vertexSrc, fragmentSrc); break;
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
			String typeString = source.substr(begin, eol - begin);
			ATN_CORE_ASSERT(typeString == "VERTEX_SHADER" || typeString == "FRAGMENT_SHADER" || 
				typeString == "PIXEL_SHADER" || typeString == "GEOMETRY_SHADER" || typeString == "COMPUTE_SHADER",
				"Invalid Shader Type specifier");
			
			ShaderType type = ShaderTypeFromString(typeString);

			uint64 nextLinePos = source.find_first_not_of("\r,\n", eol);
			pos = source.find(typeToken, nextLinePos);
			shaderSources[type] = source.substr(nextLinePos, pos - (nextLinePos == String::npos ? source.size() - 1 : nextLinePos));

			AddExternalDefines(shaderSources[type], type);
		}

		return shaderSources;
	}

	void Shader::AddExternalDefines(String& source, ShaderType type)
	{
		const char* typeToken = "#version";			// TODO: fix
		uint64 typeTokenLength = strlen(typeToken);
		uint64 pos = source.find(typeToken, 0);
		if (pos == String::npos)
		{
			ATN_CORE_ERROR("Shader '{}' does not have '#version' macro", GetName());
			return;
		}

		uint64 eol = source.find_first_of("\r\n", pos);

		String defines = "\r\n";
		
#define DEFINE(name, value) std::format("\r\n#define {} {}", name, value)

		if (type == ShaderType::FRAGMENT_SHADER)
		{
			defines += DEFINE("ALBEDO_MAP_BINDER", (int32_t)TextureBinder::ALBEDO_MAP);
			defines += DEFINE("NORMAL_MAP_BINDER", (int32_t)TextureBinder::NORMAL_MAP);
			defines += DEFINE("ROUGHNESS_MAP_BINDER", (int32_t)TextureBinder::ROUGHNESS_MAP);
			defines += DEFINE("METALNESS_MAP_BINDER", (int32_t)TextureBinder::METALNESS_MAP);
			defines += DEFINE("AO_MAP_BINDER", (int32_t)TextureBinder::AMBIENT_OCCLUSION_MAP);
			defines += DEFINE("ENVIRONMENT_MAP_BINDER", (int32_t)TextureBinder::ENVIRONMENT_MAP);
			defines += DEFINE("IRRADIANCE_MAP_BINDER", (int32_t)TextureBinder::IRRADIANCE_MAP);
			defines += DEFINE("BRDF_LUT_BINDER", (int32_t)TextureBinder::BRDF_LUT);
			defines += DEFINE("SHADOW_MAP_BINDER", (int32_t)TextureBinder::SHADOW_MAP);
			defines += DEFINE("PCF_SAMPLER_BINDER", (int32_t)TextureBinder::PCF_SAMPLER);
		}

		defines += DEFINE("MAX_DIRECTIONAL_LIGHT_COUNT", (int32_t)ShaderConstants::MAX_DIRECTIONAL_LIGHT_COUNT);
		defines += DEFINE("MAX_POINT_LIGHT_COUNT", (int32_t)ShaderConstants::MAX_POINT_LIGHT_COUNT);
		defines += DEFINE("SHADOW_CASCADES_COUNT", (int32_t)ShaderConstants::SHADOW_CASCADES_COUNT);
		defines += DEFINE("MAX_NUM_BONES_PER_VERTEX", (int32_t)ShaderConstants::MAX_NUM_BONES_PER_VERTEX);
		defines += DEFINE("MAX_NUM_BONES", (int32_t)ShaderConstants::MAX_NUM_BONES);
		defines += DEFINE("MAX_SKYBOX_MAP_LOD", (int32_t)ShaderConstants::MAX_SKYBOX_MAP_LOD);

		defines += DEFINE("RENDERER2D_CAMERA_BUFFER_BINDER", (int32_t)BufferBinder::RENDERER2D_CAMERA_DATA);
		defines += DEFINE("CAMERA_BUFFER_BINDER", (int32_t)BufferBinder::CAMERA_DATA);
		defines += DEFINE("SCENE_BUFFER_BINDER", (int32_t)BufferBinder::SCENE_DATA);
		defines += DEFINE("ENVMAP_BUFFER_BINDER", (int32_t)BufferBinder::ENVIRONMENT_MAP_DATA);
		defines += DEFINE("ENTITY_BUFFER_BINDER", (int32_t)BufferBinder::ENTITY_DATA);
		defines += DEFINE("MATERIAL_BUFFER_BINDER", (int32_t)BufferBinder::MATERIAL_DATA);
		defines += DEFINE("SHADOWS_BUFFER_BINDER", (int32_t)BufferBinder::SHADOWS_DATA);
		defines += DEFINE("LIGHT_BUFFER_BINDER", (int32_t)BufferBinder::LIGHT_DATA);
		defines += DEFINE("BONES_BUFFER_BINDER", (int32_t)BufferBinder::BONES_DATA);

		defines += DEFINE("PI", 3.14159265358979323846);

		defines += "\r\n";

#undef DEFINE

		source.insert(eol, defines);
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


	Ref<ComputeShader> ComputeShader::Create(const FilePath& path, const Vector3i& workGroupSize)
	{
		FilePath stem = path;

		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			return CreateRef<GLComputeShader>(stem.concat(L".glsl"), workGroupSize); break;
		case RendererAPI::API::None:
			ATN_CORE_ASSERT(false, "Renderer API None is not supported");
		}

		ATN_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}



	void ShaderLibrary::Add(const String& name, const Ref<IncludeShader>& shader)
	{
		ATN_CORE_ASSERT(!Exists(name), "Shader already exists!");
		m_IncludeShaders[name] = shader;
	}

	void ShaderLibrary::Add(const String& name, const Ref<Shader>& shader)
	{
		ATN_CORE_ASSERT(!Exists(name), "Shader already exists!");
		m_Shaders[name] = shader;
	}

	void ShaderLibrary::Add(const String& name, const Ref<ComputeShader>& shader)
	{
		ATN_CORE_ASSERT(!Exists(name), "Shader already exists!");
		m_ComputeShaders[name] = shader;
	}

	template <>
	Ref<IncludeShader> ShaderLibrary::Load<IncludeShader>(const String& name, const FilePath& path)
	{
		auto shader = IncludeShader::Create(path);
		Add(name, shader);
		return shader;
	}

	template <>
	Ref<Shader> ShaderLibrary::Load<Shader>(const String& name, const FilePath& path)
	{
		auto shader = Shader::Create(path);
		Add(name, shader);
		return shader;
	}

	template <>
	Ref<ComputeShader> ShaderLibrary::Load<ComputeShader>(const String& name, const FilePath& path)
	{
		auto shader = ComputeShader::Create(path);
		Add(name, shader);
		return shader;
	}

	template <>
	Ref<IncludeShader> ShaderLibrary::Get<IncludeShader>(const String& name)
	{
		ATN_CORE_ASSERT(Exists(name), "Shader not found!");
		return m_IncludeShaders.at(name);
	}

	template <>
	Ref<Shader> ShaderLibrary::Get<Shader>(const String& name)
	{
		ATN_CORE_ASSERT(Exists(name), "Shader not found!");
		return m_Shaders.at(name);
	}

	template <>
	Ref<ComputeShader> ShaderLibrary::Get<ComputeShader>(const String& name)
	{
		ATN_CORE_ASSERT(Exists(name), "Shader not found!");
		return m_ComputeShaders.at(name);
	}

	bool ShaderLibrary::Exists(const String& name)
	{
		return (m_Shaders.find(name) != m_Shaders.end()) || 
			(m_IncludeShaders.find(name) != m_IncludeShaders.end()) ||
			(m_ComputeShaders.find(name) != m_ComputeShaders.end());
	}

	void ShaderLibrary::Reload()
	{
		for (const auto& [key, shader] : m_IncludeShaders)
			shader->Reload();

		for (const auto& [key, shader] : m_Shaders)
			shader->Reload();

		for (const auto& [key, shader] : m_ComputeShaders)
			shader->Reload();
	}
}
