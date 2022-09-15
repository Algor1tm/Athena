#include "atnpch.h"
#include "OpenGLShader.h"

#include <glad/glad.h>


namespace Athena
{
	static GLenum ShaderTypeFromString(const String& type)
	{
		if (type == "VERTEX_SHADER") return GL_VERTEX_SHADER;
		if (type == "FRAGMENT_SHADER" || type == "PIXEL_SHADER") return GL_FRAGMENT_SHADER;

		ATN_CORE_ASSERT(false, "Unknown shader type");
		return 0;
	}

	OpenGLShader::OpenGLShader(const String& filepath)
	{
		String result = ReadFile(filepath);
		auto shaderSources = PreProcess(result);
		Compile(shaderSources);

		// assets/shaders/Grid.glsl -> m_Name = Grid
		SIZE_T lastSlash = filepath.find_last_of("/\\");
		lastSlash = lastSlash == std::string::npos ? 0 : lastSlash + 1;
		SIZE_T lastDot = filepath.rfind('.');
		SIZE_T count = lastDot == std::string::npos ? filepath.size() - lastSlash : lastDot - lastSlash;
		m_Name = filepath.substr(lastSlash, count);
	}

	OpenGLShader::OpenGLShader(const String& name, const String& vertexSrc, const String& fragmentSrc)
	{
		std::unordered_map<GLenum, String> sources;
		sources[GL_VERTEX_SHADER] = vertexSrc;
		sources[GL_FRAGMENT_SHADER] = fragmentSrc;
		Compile(sources);

		m_Name = name;
	}

	OpenGLShader::~OpenGLShader()
	{
		glDeleteProgram(m_RendererID);
	}

	String OpenGLShader::ReadFile(const String& filepath)
	{
		String result;
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

	std::unordered_map<GLenum, String> OpenGLShader::PreProcess(const String& source)
	{
		std::unordered_map<GLenum, String> shaderSources;

		const char* typeToken = "#type";
		SIZE_T typeTokenLength = strlen(typeToken);
		SIZE_T pos = source.find(typeToken, 0);
		while (pos != std::string::npos)
		{
			SIZE_T eol = source.find_first_of("\r\n", pos);
			ATN_CORE_ASSERT(eol != std::string::npos, "Syntax Error");
			SIZE_T begin = pos + typeTokenLength + 1;
			String type = source.substr(begin, eol - begin);
			ATN_CORE_ASSERT(type == "VERTEX_SHADER" || type == "FRAGMENT_SHADER" || type == "PIXEL_SHADER", 
				"Invalid Shader Type specifier");

			SIZE_T nextLinePos = source.find_first_not_of("\r,\n", eol);
			pos = source.find(typeToken, nextLinePos);
			shaderSources[ShaderTypeFromString(type)] = 
				source.substr(nextLinePos, pos - (nextLinePos == std::string::npos ? source.size() - 1: nextLinePos));
		}

		return shaderSources;
	}

	void OpenGLShader::Compile(const std::unordered_map<GLenum, String>& shaderSources)
	{
		GLuint program = glCreateProgram();

		ATN_CORE_ASSERT(shaderSources.size() < 3, "Engine supports only up to 2 shaders");
		std::array<GLenum, 2> shaderIDs;
		int shaderIDIndex = 0;

		for (auto&& [glType, sourceString] : shaderSources)
		{
			GLuint shader = glCreateShader(glType);

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

				ATN_CORE_ERROR("{0}", infoLog.data());
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

	void OpenGLShader::Bind() const
	{
		glUseProgram(m_RendererID);
	}

	void OpenGLShader::UnBind() const
	{
		glUseProgram(0);
	}

	int OpenGLShader::GetUniformLocation(const String& name)
	{
		if (m_UniformLocationCache.find(name) != m_UniformLocationCache.end())
			return m_UniformLocationCache[name];

		int location = glGetUniformLocation(m_RendererID, name.c_str());
		if (location == -1)
			ATN_CORE_WARN("Uniform name: '{0}' does not exist!", name.c_str());
		m_UniformLocationCache[name] = location;

		return location;
	}

	void OpenGLShader::SetInt(const String& name, int value)
	{
		UploadUniformInt(name, value);
	}

	void OpenGLShader::SetIntArray(const String& name, int* value, uint32 count)
	{
		UploadUniformIntArray(name, value, count);
	}

	void OpenGLShader::SetFloat(const String& name, float value)
	{
		UploadUniformFloat(name, value);
	}

	void OpenGLShader::SetFloat3(const String& name, const Vector3& vec3)
	{
		UploadUniformFloat3(name, vec3);
	}

	void OpenGLShader::SetFloat4(const String& name, const Vector4& vec4)
	{
		UploadUniformFloat4(name, vec4);
	}

	void OpenGLShader::SetMat4(const String& name, const Matrix4& mat4)
	{
		UploadUniformMat4(name, mat4);
	}

	void OpenGLShader::UploadUniformInt(const String& name, int value)
	{
		GLint location = GetUniformLocation(name.data());
		glUniform1i(location, value);
	}

	void OpenGLShader::UploadUniformIntArray(const String& name, int* value, uint32 count)
	{
		GLint location = GetUniformLocation(name.data());
		glUniform1iv(location, count, value);
	}

	void OpenGLShader::UploadUniformFloat(const String& name, float value)
	{
		GLint location = GetUniformLocation(name.data());
		glUniform1f(location, value);
	}

	void OpenGLShader::UploadUniformFloat2(const String& name, const Vector2& vec2)
	{
		GLint location = GetUniformLocation(name.data());
		glUniform2f(location, vec2.x, vec2.y);
	}

	void OpenGLShader::UploadUniformFloat3(const String& name, const Vector3& vec3)
	{
		GLint location = GetUniformLocation(name.data());
		glUniform3f(location, vec3.x, vec3.y, vec3.z);
	}

	void OpenGLShader::UploadUniformFloat4(const String& name, const Vector4& vec4)
	{
		GLint location = GetUniformLocation(name.data());
		glUniform4f(location, vec4.x, vec4.y, vec4.z, vec4.w);
	}

	void OpenGLShader::UploadUniformMat3(const String& name, const Matrix3& mat3)
	{
		GLint location = GetUniformLocation(name.data());
		glUniformMatrix3fv(location, 1, GL_FALSE, mat3.Data());
	}

	void OpenGLShader::UploadUniformMat4(const String& name, const Matrix4& mat4)
	{
		GLint location = GetUniformLocation(name.data());
		glUniformMatrix4fv(location, 1, GL_FALSE, mat4.Data());
	}
}
