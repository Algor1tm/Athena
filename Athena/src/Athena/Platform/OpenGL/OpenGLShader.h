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
		OpenGLShader(const std::string& filepath);
		OpenGLShader(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc);
		~OpenGLShader();

		void Bind() const override;
		void UnBind() const override;
		const std::string& GetName() const override { return m_Name; };

		void SetInt(const std::string& name, int value) override;
		void SetIntArray(const std::string& name, int* value, uint32_t count) override;
		void SetFloat(const std::string& name, float value) override;
		void SetFloat3(const std::string& name, const Vector3& vec3) override;
		void SetFloat4(const std::string& name, const Vector4& vec4) override;
		void SetMat4(const std::string& name, const Matrix4& mat4) override;

		void UploadUniformInt(const std::string& name, int value);
		void UploadUniformIntArray(const std::string& name, int* value, uint32_t count);
		void UploadUniformFloat(const std::string& name, float value);
		void UploadUniformFloat2(const std::string& name, const Vector2& vec2);
		void UploadUniformFloat3(const std::string& name, const Vector3& vec3);
		void UploadUniformFloat4(const std::string& name, const Vector4& vec4);
		void UploadUniformMat3(const std::string& name, const Matrix3& mat3);
		void UploadUniformMat4(const std::string& name, const Matrix4& mat4);

	private:
		std::string ReadFile(const std::string& filepath);	
		std::unordered_map<GLenum, std::string> PreProcess(const std::string& source);
		void Compile(const std::unordered_map<GLenum, std::string>& shaderSources);

		int GetUniformLocation(const std::string& name);

	private:
		RendererID m_RendererID = 0;
		std::string m_Name;
		std::unordered_map<std::string, int> m_UniformLocationCache;
	};
}