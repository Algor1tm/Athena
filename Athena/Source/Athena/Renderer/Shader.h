#pragma once

#include "Athena/Math/Vector.h"
#include "Athena/Math/Matrix.h"


namespace Athena
{
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

		virtual const String& GetName() const = 0;

		static Ref<Shader> Create(const String& filepath);
		static Ref<Shader> Create(const String& name, const String& vertexSrc, const String& fragmentSrc);

	protected:
		uint32 m_RendererID;
	};

	class ATHENA_API ShaderLibrary
	{
	public:
		void Add(const Ref<Shader>& shader);
		void Add(const String& name, const Ref<Shader>& shader);
		Ref<Shader> Load(const String& filepath);
		Ref<Shader> Load(const String& name, const String& filepath);

		Ref<Shader> Get(const String& name);
		bool Exists(const String& name);

	private:
		std::unordered_map<String, Ref<Shader>> m_Shaders;
	};
}
