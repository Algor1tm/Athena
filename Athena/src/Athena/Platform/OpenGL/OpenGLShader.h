#pragma once

#include "Athena/Renderer/Shader.h"

#include "Athena/Math/Matrix.h"
#include "Athena/Math/Vector.h"


namespace Athena
{
	class ATHENA_API OpenGLShader: public Shader
	{
	public:
		OpenGLShader(const std::string& vertexSrc, const std::string& fragmentSrc);
		~OpenGLShader();

		void Bind() const override;
		void UnBind() const override;

		void UploadUniformInt(const std::string& name, int value);
		void UploadUniformFloat(const std::string& name, float value);
		void UploadUniformFloat2(const std::string& name, const Vector2& vec2);
		void UploadUniformFloat3(const std::string& name, const Vector3& vec3);
		void UploadUniformFloat4(const std::string& name, const Vector4& vec4);
		void UploadUniformMat3(const std::string& name, const Matrix3& mat3);
		void UploadUniformMat4(const std::string& name, const Matrix4& mat4);

	private:
		uint32_t m_RendererID;
	};
}