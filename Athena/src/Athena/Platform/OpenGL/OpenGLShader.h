#pragma once

#include "Athena/Renderer/Shader.h"

#include "Athena/Math/Matrix.h"
#include "Athena/Math/Vector.h"

// TODO: Remove
typedef unsigned int GLenum;


namespace Athena
{
	class ATHENA_API OpenGLShader: public Shader
	{
	public:
		OpenGLShader(const String& filepath);
		OpenGLShader(const String& name, const String& vertexSrc, const String& fragmentSrc);
		~OpenGLShader();

		void Bind() const override;
		void UnBind() const override;
		const String& GetName() const override { return m_Name; };

		void SetInt(const String& name, int value) override;
		void SetIntArray(const String& name, int* value, uint32 count) override;
		void SetFloat(const String& name, float value) override;
		void SetFloat3(const String& name, const Vector3& vec3) override;
		void SetFloat4(const String& name, const Vector4& vec4) override;
		void SetMat4(const String& name, const Matrix4& mat4) override;

		void UploadUniformInt(const String& name, int value);
		void UploadUniformIntArray(const String& name, int* value, uint32 count);
		void UploadUniformFloat(const String& name, float value);
		void UploadUniformFloat2(const String& name, const Vector2& vec2);
		void UploadUniformFloat3(const String& name, const Vector3& vec3);
		void UploadUniformFloat4(const String& name, const Vector4& vec4);
		void UploadUniformMat3(const String& name, const Matrix3& mat3);
		void UploadUniformMat4(const String& name, const Matrix4& mat4);

	private:
		String ReadFile(const String& filepath);	
		std::unordered_map<GLenum, String> PreProcess(const String& source);
		void Compile(const std::unordered_map<GLenum, String>& shaderSources);

		int GetUniformLocation(const String& name);

	private:
		RendererID m_RendererID = 0;
		String m_Name;
		std::unordered_map<String, int> m_UniformLocationCache;
	};
}