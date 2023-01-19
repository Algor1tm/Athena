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

		m_Filepath = filepath;
		m_Name = m_Filepath.stem().string();

		Reload();
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

	bool GLShader::Compile(const std::unordered_map<ShaderType, String>& shaderSources)
	{
		GLuint program = glCreateProgram();

		ATN_CORE_ASSERT(shaderSources.size() < 3, "Engine supports only up to 2 shaders");
		std::array<GLenum, 2> shaderIDs;
		int shaderIDIndex = 0;

		m_Compiled = true;
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

				ATN_CORE_ERROR("{0} '{1}' compilation failed!", ShaderTypeToString(glType), m_Name);
				ATN_CORE_WARN("Errors/Warnings:\n{}", infoLog.data());

				m_Compiled = false;
				return m_Compiled;
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

			m_Compiled = false;
			return m_Compiled;
		}

		for (auto id : shaderIDs)
			glDetachShader(m_RendererID, id);

		return m_Compiled;
	}

	void GLShader::Reload()
	{
		String result = ReadFile(m_Filepath);
		auto shaderSources = PreProcess(result);
		Compile(shaderSources);
	}

	void GLShader::Bind() const
	{
		if (m_Compiled)
			glUseProgram(m_RendererID);
		else
			glUseProgram(0);
	}

	void GLShader::UnBind() const
	{
		glUseProgram(0);
	}
}
