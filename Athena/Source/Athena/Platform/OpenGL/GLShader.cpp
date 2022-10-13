#include "GLShader.h"

#include <glad/glad.h>


namespace Athena
{
	static GLenum ShaderTypeToGLenum(ShaderType type)
	{
		switch (type)
		{
		case ShaderType::VERTEX_SHADER: return GL_VERTEX_SHADER;
		case ShaderType::FRAGMENT_SHADER: return GL_FRAGMENT_SHADER;
		}

		ATN_CORE_ASSERT(false, "Unknown shader type!");
		return 0;
	}

	GLShader::GLShader(const Filepath& filepath)
	{
		ATN_CORE_ASSERT(std::filesystem::exists(filepath), "Invalid filepath for Shader");
		const auto& strFilepath = filepath.string();

		SetNameFromFilepath(strFilepath);

		String result = ReadFile(strFilepath);
		auto shaderSources = PreProcess(result);
		Compile(shaderSources);
	}

	GLShader::GLShader(const String& name, const String& vertexSrc, const String& fragmentSrc)
	{
		m_Name = name;

		std::unordered_map<ShaderType, String> sources;
		sources[ShaderType::VERTEX_SHADER] = vertexSrc;
		sources[ShaderType::FRAGMENT_SHADER] = fragmentSrc;
		Compile(sources);
	}

	GLShader::~GLShader()
	{
		glDeleteProgram(m_RendererID);
	}

	void GLShader::Compile(const std::unordered_map<ShaderType, String>& shaderSources)
	{
		GLuint program = glCreateProgram();

		ATN_CORE_ASSERT(shaderSources.size() < 3, "Engine supports only up to 2 shaders");
		std::array<GLenum, 2> shaderIDs;
		int shaderIDIndex = 0;

		for (auto&& [glType, sourceString] : shaderSources)
		{
			GLuint shader = glCreateShader(ShaderTypeToGLenum(glType));

			const char* source = sourceString.c_str();
			glShaderSource(shader, 1, &source, 0);

			glCompileShader(shader);

			GLint isCompiled = 0;
			glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
			if (isCompiled == GL_FALSE)
			{
				GLint maxLength = 0;
				glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

				std::vector<GLchar> infoLog(maxLength);
				glGetShaderInfoLog(shader, maxLength, &maxLength, infoLog.data());

				glDeleteShader(shader);

				ATN_CORE_WARN("Shader '{0}':\n{1}", m_Name, infoLog.data());
				ATN_CORE_ASSERT(false, "Shader compilation failed!");
				break;
			}

			glAttachShader(program, shader);
			shaderIDs[shaderIDIndex++] = shader;
		}

		m_RendererID = program;
		glLinkProgram(m_RendererID);

		GLint isLinked = 0;
		glGetProgramiv(m_RendererID, GL_LINK_STATUS, &isLinked);
		if (isLinked == GL_FALSE)
		{
			GLint maxLength = 0;
			glGetProgramiv(m_RendererID, GL_INFO_LOG_LENGTH, &maxLength);

			std::vector<GLchar> infoLog(maxLength);
			glGetProgramInfoLog(m_RendererID, maxLength, &maxLength, infoLog.data());

			for(auto id: shaderIDs)
				glDeleteShader(id);

			ATN_CORE_ERROR("{0}", infoLog.data());
			ATN_CORE_ASSERT(false, "Program linking failure!");
			return;
		}

		for (auto id : shaderIDs)
			glDetachShader(m_RendererID, id);
	}

	void GLShader::Bind() const
	{
		glUseProgram(m_RendererID);
	}

	void GLShader::UnBind() const
	{
		glUseProgram(0);
	}

	int GLShader::GetUniformLocation(const String& name)
	{
		if (m_UniformLocationCache.find(name) != m_UniformLocationCache.end())
			return m_UniformLocationCache[name];

		int location = glGetUniformLocation(m_RendererID, name.c_str());
		if (location == -1)
			ATN_CORE_WARN("Uniform name: '{0}' does not exist!", name.c_str());
		m_UniformLocationCache[name] = location;

		return location;
	}

	void GLShader::SetInt(const String& name, int value)
	{
		UploadUniformInt(name, value);
	}

	void GLShader::SetIntArray(const String& name, int* value, uint32 count)
	{
		UploadUniformIntArray(name, value, count);
	}

	void GLShader::SetFloat(const String& name, float value)
	{
		UploadUniformFloat(name, value);
	}

	void GLShader::SetFloat3(const String& name, const Vector3& vec3)
	{
		UploadUniformFloat3(name, vec3);
	}

	void GLShader::SetFloat4(const String& name, const Vector4& vec4)
	{
		UploadUniformFloat4(name, vec4);
	}

	void GLShader::SetMat4(const String& name, const Matrix4& mat4)
	{
		UploadUniformMat4(name, mat4);
	}

	void GLShader::UploadUniformInt(const String& name, int value)
	{
		GLint location = GetUniformLocation(name.data());
		glUniform1i(location, value);
	}

	void GLShader::UploadUniformIntArray(const String& name, int* value, uint32 count)
	{
		GLint location = GetUniformLocation(name.data());
		glUniform1iv(location, count, value);
	}

	void GLShader::UploadUniformFloat(const String& name, float value)
	{
		GLint location = GetUniformLocation(name.data());
		glUniform1f(location, value);
	}

	void GLShader::UploadUniformFloat2(const String& name, const Vector2& vec2)
	{
		GLint location = GetUniformLocation(name.data());
		glUniform2f(location, vec2.x, vec2.y);
	}

	void GLShader::UploadUniformFloat3(const String& name, const Vector3& vec3)
	{
		GLint location = GetUniformLocation(name.data());
		glUniform3f(location, vec3.x, vec3.y, vec3.z);
	}

	void GLShader::UploadUniformFloat4(const String& name, const Vector4& vec4)
	{
		GLint location = GetUniformLocation(name.data());
		glUniform4f(location, vec4.x, vec4.y, vec4.z, vec4.w);
	}

	void GLShader::UploadUniformMat3(const String& name, const Matrix3& mat3)
	{
		GLint location = GetUniformLocation(name.data());
		glUniformMatrix3fv(location, 1, GL_FALSE, mat3.Data());
	}

	void GLShader::UploadUniformMat4(const String& name, const Matrix4& mat4)
	{
		GLint location = GetUniformLocation(name.data());
		glUniformMatrix4fv(location, 1, GL_FALSE, mat4.Data());
	}
}
