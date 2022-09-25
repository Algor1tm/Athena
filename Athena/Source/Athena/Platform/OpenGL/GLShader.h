#pragma once

#include "Athena/Renderer/Shader.h"

#include "Athena/Math/Matrix.h"
#include "Athena/Math/Vector.h"

// TODO: Remove
typedef unsigned int GLenum;


namespace Athena
{
	class ATHENA_API GLShader: public Shader
	{
	public:
		GLShader(const Filepath& filepath);
		GLShader(const String& name, const String& vertexSrc, const String& fragmentSrc);
		virtual ~GLShader();

		virtual void Bind() const override;
		virtual void UnBind() const override;

		virtual void SetInt(const String& name, int value) override;
		virtual void SetIntArray(const String& name, int* value, uint32 count) override;
		virtual void SetFloat(const String& name, float value) override;
		virtual void SetFloat3(const String& name, const Vector3& vec3) override;
		virtual void SetFloat4(const String& name, const Vector4& vec4) override;
		virtual void SetMat4(const String& name, const Matrix4& mat4) override;

		void UploadUniformInt(const String& name, int value);
		void UploadUniformIntArray(const String& name, int* value, uint32 count);
		void UploadUniformFloat(const String& name, float value);
		void UploadUniformFloat2(const String& name, const Vector2& vec2);
		void UploadUniformFloat3(const String& name, const Vector3& vec3);
		void UploadUniformFloat4(const String& name, const Vector4& vec4);
		void UploadUniformMat3(const String& name, const Matrix3& mat3);
		void UploadUniformMat4(const String& name, const Matrix4& mat4);

	private:
		void Compile(const std::unordered_map<ShaderType, String>& shaderSources);

		int GetUniformLocation(const String& name);

	private:
		uint32 m_RendererID = 0;
		std::unordered_map<String, int> m_UniformLocationCache;
	};
}
