#include "Shader.h"

#include "Athena/Renderer/Renderer.h"

#include "Athena/Platform/Vulkan/VulkanShader.h"

#include <format>


namespace Athena
{
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

	Ref<ShaderPack> ShaderPack::Create(const FilePath& path)
	{
		Ref<ShaderPack> result = Ref<ShaderPack>::Create();
		result->m_Directory = path;
		result->LoadDirectory(path);
		return result;
	}

	void ShaderPack::LoadDirectory(const FilePath& path)
	{
		for (const auto& dirEntry : std::filesystem::directory_iterator(path))
		{
			const FilePath& path = dirEntry.path();

			if (dirEntry.is_directory())
				LoadDirectory(path);

			if (path.extension() == ".glsl" || path.extension() == ".hlsl")
			{
				String name = path.stem().string();
				Ref<Shader> shader = Shader::Create(path, name);
				Add(name, shader);
			}
		}
	}

	void ShaderPack::Add(const String& name, const Ref<Shader>& shader)
	{
		ATN_CORE_ASSERT(!Exists(name), "Shader already exists!");
		m_Shaders[name] = shader;
	}

	Ref<Shader> ShaderPack::Load(const FilePath& path)
	{
		auto shader = Shader::Create(m_Directory / path);
		Add(shader->GetName(), shader);
		return shader;
	}

	Ref<Shader> ShaderPack::Load(const FilePath& path, const String& name)
	{
		auto shader = Shader::Create(m_Directory / path, name);
		Add(shader->GetName(), shader);
		return shader;
	}

	Ref<Shader> ShaderPack::Get(const String& name)
	{
		ATN_CORE_ASSERT(Exists(name), "Shader not found!");
		return m_Shaders.at(name);
	}

	bool ShaderPack::Exists(const String& name)
	{
		return (m_Shaders.find(name) != m_Shaders.end());
	}

	void ShaderPack::Reload()
	{
		for (const auto& [key, shader] : m_Shaders)
			shader->Reload();
	}
}
