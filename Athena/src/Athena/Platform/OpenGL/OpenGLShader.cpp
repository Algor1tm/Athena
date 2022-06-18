#include "atnpch.h"
#include "OpenGLShader.h"

#include <glad/glad.h>


namespace Athena
{
	static GLenum ShaderTypeFromString(const std::string& type)
	{
		if (type == "VERTEX_SHADER") return GL_VERTEX_SHADER;
		if (type == "FRAGMENT_SHADER" || type == "PIXEL_SHADER") return GL_FRAGMENT_SHADER;

		ATN_CORE_ASSERT(false, "Unknown shader type '{0}'", type.data());
		return 0;
	}

	OpenGLShader::OpenGLShader(const std::string& filepath)
	{
		std::string result = ReadFile(filepath);
		auto shaderSources = PreProcess(result);
		Compile(shaderSources);
	}

	OpenGLShader::OpenGLShader(const std::string& vertexSrc, const std::string& fragmentSrc)
	{
		std::unordered_map<GLenum, std::string> sources;
		sources[GL_VERTEX_SHADER] = vertexSrc;
		sources[GL_FRAGMENT_SHADER] = fragmentSrc;
		Compile(sources);
	}

	OpenGLShader::~OpenGLShader()
	{
		glDeleteProgram(m_RendererID);
	}

	std::string OpenGLShader::ReadFile(const std::string& filepath)
	{
		std::string result;
		std::ifstream in(filepath, std::ios::in | std::ios::binary);
		if (in)
		{
			in.seekg(0, std::ios::end);
			result.resize(in.tellg());
			in.seekg(0, std::ios::beg);
			in.read(result.data(), result.size());
			in.close();
		}
		else
		{
			ATN_CORE_ERROR("Could not open file: '{0}'", filepath);
		}
		return result;
	}

	std::unordered_map<GLenum, std::string> OpenGLShader::PreProcess(const std::string& source)
	{ 
		std::unordered_map<GLenum, std::string> shaderSources;

		const char* typeToken = "#type";
		size_t typeTokenLength = strlen(typeToken);
		size_t pos = source.find(typeToken, 0);
		while (pos != std::string::npos)
		{
			size_t eol = source.find_first_of("\r\n", pos);
			ATN_CORE_ASSERT(eol != std::string::npos, "Syntax Error");
			size_t begin = pos + typeTokenLength + 1;
			std::string type = source.substr(begin, eol - begin);
			ATN_CORE_ASSERT(type == "VERTEX_SHADER" || type == "FRAGMENT_SHADER" || type == "PIXEL_SHADER", 
				"Invalid Shader Type specifier");

			size_t nextLinePos = source.find_first_not_of("\r,\n", eol);
			pos = source.find(typeToken, nextLinePos);
			shaderSources[ShaderTypeFromString(type)] = 
				source.substr(
					nextLinePos, pos - (nextLinePos == std::string::npos ? source.size() - 1: nextLinePos));
		}

		return shaderSources;
	}

	void OpenGLShader::Compile(const std::unordered_map<GLenum, std::string>& shaderSources)
	{
		GLuint program = glCreateProgram();
		std::vector<GLenum> shaderIDs(shaderSources.size());

		for (auto&& [glType, sourceString] : shaderSources)
		{
			GLuint shader = glCreateShader(glType);

			const char* source = sourceString.c_str();
			glShaderSource(shader, 1, &(const GLchar*)source, 0);

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

				ATN_CORE_ERROR("{0}", infoLog.data());
				ATN_CORE_ASSERT(false, "Shader compilation failed!");
				break;
			}

			glAttachShader(program, shader);
			shaderIDs.push_back(shader);
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

	void OpenGLShader::Bind() const
	{
		glUseProgram(m_RendererID);
	}

	void OpenGLShader::UnBind() const
	{
		glUseProgram(0);
	}

	void OpenGLShader::UploadUniformInt(const std::string& name, int value)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.data());
		glUniform1i(location, value);
	}

	void OpenGLShader::UploadUniformFloat(const std::string& name, float value)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.data());
		glUniform1f(location, value);
	}

	void OpenGLShader::UploadUniformFloat2(const std::string& name, const Vector2& vec2)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.data());
		glUniform2f(location, vec2.x, vec2.y);
	}

	void OpenGLShader::UploadUniformFloat3(const std::string& name, const Vector3& vec3)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.data());
		glUniform3f(location, vec3.x, vec3.y, vec3.z);
	}

	void OpenGLShader::UploadUniformFloat4(const std::string& name, const Vector4& vec4)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.data());
		glUniform4f(location, vec4.x, vec4.y, vec4.z, vec4.w);
	}

	void OpenGLShader::UploadUniformMat3(const std::string& name, const Matrix3& mat3)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.data());
		glUniformMatrix3fv(location, 1, GL_FALSE, mat3.Data());
	}

	void OpenGLShader::UploadUniformMat4(const std::string& name, const Matrix4& mat4)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.data());
		glUniformMatrix4fv(location, 1, GL_FALSE, mat4.Data());
	}
}
