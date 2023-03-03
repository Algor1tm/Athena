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
		GEOMETRY_SHADER = 2,
		COMPUTE_SHADER = 3
	};

	inline std::string_view ShaderTypeToString(ShaderType type)
	{
		switch (type)
		{
		case ShaderType::VERTEX_SHADER: return "Vertex Shader";
		case ShaderType::FRAGMENT_SHADER: return "Fragment Shader";
		case ShaderType::GEOMETRY_SHADER: return "Geometry Shader";
		case ShaderType::COMPUTE_SHADER: return "Compute Shader";
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
		void AddExternalDefines(String& source, ShaderType type);

	protected:
		String m_Name;
		FilePath m_FilePath;
	};


	class ATHENA_API IncludeShader: public Shader
	{
	public:
		static Ref<IncludeShader> Create(const FilePath& path);
		virtual ~IncludeShader() = default;

	private:
		virtual void Bind() const override {};
		virtual void UnBind() const override {};
	};


	class ATHENA_API ComputeShader : public Shader 
	{
	public:
		static Ref<ComputeShader> Create(const FilePath& path, const Vector3i& workGroupSize = { 8, 4, 1 });
		virtual ~ComputeShader() = default;

		virtual void Execute(uint32 x, uint32 y, uint32 z = 1) = 0;
	};


	class ATHENA_API ShaderLibrary
	{
	public:
		void Add(const String& name, const Ref<IncludeShader>& shader);
		void Add(const String& name, const Ref<Shader>& shader);
		void Add(const String& name, const Ref<ComputeShader>& shader);

		template <typename T>
		Ref<T> Load(const String& name, const FilePath& path);
		
		template <typename T>
		Ref<T> Get(const String& name);

		bool Exists(const String& name);

		void Reload();

	private:
		std::unordered_map<String, Ref<IncludeShader>> m_IncludeShaders;
		std::unordered_map<String, Ref<Shader>> m_Shaders;
		std::unordered_map<String, Ref<ComputeShader>> m_ComputeShaders;
	};
}
