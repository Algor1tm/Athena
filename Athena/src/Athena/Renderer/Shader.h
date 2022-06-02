#pragma once


namespace Athena
{
	class ATHENA_API Shader
	{
	public:
		Shader(const std::string& vertexSrc, const std::string& fragmentSrc);
		~Shader();

		void Bind() const;
		void UnBind() const;

	private:
		uint32_t m_RendererID;
	};
}
