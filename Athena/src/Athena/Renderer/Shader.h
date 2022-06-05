#pragma once

#include "Athena/Math/Matrix.h"


namespace Athena
{
	class ATHENA_API Shader
	{
	public:
		Shader(const std::string& vertexSrc, const std::string& fragmentSrc);
		~Shader();

		void Bind() const;
		void UnBind() const;

		void UploadUniformMat4(const std::string& name, const Matrix4& mat4);

	private:
		uint32_t m_RendererID;
	};
}
