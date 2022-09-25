#pragma once

#include "Athena/Math/Vector.h"
#include "Athena/Math/Matrix.h"

#include "Buffer.h"


namespace Athena
{
	enum class ShaderType
	{
		VERTEX_SHADER = 0,
		FRAGMENT_SHADER = 1,
	};


	class ATHENA_API Shader
	{
	public:
		virtual ~Shader() = default;

		virtual void Bind() const = 0;
		virtual void UnBind() const = 0;

		virtual void SetInt(const String& name, int value) = 0;
		virtual void SetIntArray(const String& name, int* value, uint32 count) = 0;
		virtual void SetFloat(const String& name, float value) = 0;
		virtual void SetFloat3(const String& name, const Vector3& vec3) = 0;
		virtual void SetFloat4(const String& name, const Vector4& vec4) = 0;
		virtual void SetMat4(const String& name, const Matrix4& mat4) = 0;

		const String& GetName() const { return m_Name; }

		// Pass file without extension
		// if RendererAPI = Direct3D extension will be .hlsl
		// if RendererAPI = OpenGL extension will be .glsl
		static Ref<Shader> Create(const BufferLayout& layout, const Filepath& filepath);
		static Ref<Shader> Create(const BufferLayout& layout, const String& name, const String& vertexSrc, const String& fragmentSrc);

	protected:
		String ReadFile(const String& filepath);
		std::unordered_map<ShaderType, String> PreProcess(const String& source);

		void SetNameFromFilepath(const String& filepath);

	protected:
		String m_Name;
	};

	class ATHENA_API ShaderLibrary
	{
	public:
		void Add(const Ref<Shader>& shader);
		void Add(const String& name, const Ref<Shader>& shader);
		Ref<Shader> Load(const BufferLayout& layout, const String& filepath);
		Ref<Shader> Load(const BufferLayout& layout, const String& name, const String& filepath);

		Ref<Shader> Get(const String& name);
		bool Exists(const String& name);

	private:
		std::unordered_map<String, Ref<Shader>> m_Shaders;
	};
}
