#pragma once

#include "Athena/Math/Vector.h"
#include "Athena/Math/Matrix.h"

#include "Athena/Renderer/GPUBuffers.h"


namespace Athena
{
	enum class ShaderType
	{
		VERTEX_SHADER = 0,
		FRAGMENT_SHADER = 1,
	};

	inline std::string_view ShaderTypeToString(ShaderType type)
	{
		switch (type)
		{
		case ShaderType::VERTEX_SHADER: return "Vertex Shader";
		case ShaderType::FRAGMENT_SHADER: return "Fragment Shader";
		}

		ATN_CORE_ASSERT(false);
		return "";
	}


	class ATHENA_API Shader
	{
	public:
		// Pass file without extension
		//     if RendererAPI = Direct3D extension will be .hlsl
		//     if RendererAPI = OpenGL extension will be .glsl
		static Ref<Shader> Create(const FilePath& path);
		static Ref<Shader> Create(const String& name, const String& vertexSrc, const String& fragmentSrc);

		virtual ~Shader() = default;

		virtual void Bind() const = 0;
		virtual void UnBind() const = 0;

		virtual void Reload() = 0;

		const String& GetName() const { return m_Name; }

	protected:
		std::unordered_map<ShaderType, String> PreProcess(const String& source);

	protected:
		String m_Name;
		FilePath m_FilePath;
	};


	class ATHENA_API IncludeShader
	{
	public:
		static Ref<IncludeShader> Create(const FilePath& path);
		virtual ~IncludeShader() = default;

		virtual void Reload() = 0;
	};

	class ATHENA_API ShaderLibrary
	{
	public:
		void Add(const Ref<Shader>& shader);
		void Add(const String& name, const Ref<Shader>& shader);
		Ref<Shader> Load(const FilePath& filepath);
		Ref<Shader> Load(const String& name, const FilePath& filepath);

		Ref<Shader> Get(const String& name);
		bool Exists(const String& name);

	private:
		std::unordered_map<String, Ref<Shader>> m_Shaders;
	};
}
