#include "GLShader.h"

#include "Athena/Platform/OpenGL/GLUtils.h"

#include "Athena/Core/FileSystem.h"
#include "Athena/Renderer/Renderer.h"

#include <glad/glad.h>


namespace Athena
{
	static bool CompileShaderSources(const std::unordered_map<ShaderType, String>& shaderSources, const String& name, GLuint* program)
	{
		*program = glCreateProgram();

		std::vector<GLenum> shaderIDs(shaderSources.size(), -1);
		int shaderIDIndex = 0;

		for (auto&& [glType, sourceString] : shaderSources)
		{
			GLuint shader = glCreateShader(Utils::ShaderTypeToGLenum(glType));

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

				for (auto id : shaderIDs)
				{
					if (id != -1)
						glDeleteShader(id);
				}

				glDeleteProgram(*program);

				ATN_CORE_ERROR_TAG_("GLShader", "{0} '{1}' compilation failed!", ShaderTypeToString(glType), name);
				ATN_CORE_INFO("Errors/Warnings:\n{}", infoLog.data());

				return false;
			}

			glAttachShader(*program, shader);
			shaderIDs[shaderIDIndex++] = shader;
		}

		glLinkProgram(*program);

		GLint isLinked = 0;
		glGetProgramiv(*program, GL_LINK_STATUS, &isLinked);
		if (isLinked == GL_FALSE)
		{
			GLint maxLength = 0;
			glGetProgramiv(*program, GL_INFO_LOG_LENGTH, &maxLength);

			std::vector<GLchar> infoLog(maxLength);
			glGetProgramInfoLog(*program, maxLength, &maxLength, infoLog.data());

			for (auto id : shaderIDs)
				glDeleteShader(id);

			glDeleteProgram(*program);

			ATN_CORE_ERROR_TAG_("GLShader", "{0} linking failed!", name);
			ATN_CORE_INFO("Errors/Warnings: \n{0}", infoLog.data());

			return false;
		}

		for (auto id : shaderIDs)
			glDetachShader(*program, id);

		return true;
	}


	GLShader::GLShader(const FilePath& path)
	{
		m_FilePath = path;
		m_Name = m_FilePath.stem().string();

		Reload();
	}

	GLShader::GLShader(const FilePath& path, const String& name)
	{
		m_FilePath = path;
		m_Name = name;

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
		if(m_Compiled)
			glDeleteProgram(m_RendererID);
	}

	bool GLShader::Compile(const std::unordered_map<ShaderType, String>& shaderSources)
	{
		m_Compiled = CompileShaderSources(shaderSources, m_Name, &m_RendererID);
		return m_Compiled;
	}

	void GLShader::Reload()
	{
		if (!FileSystem::Exists(m_FilePath))
		{
			ATN_CORE_ERROR_TAG_("GLShader", "Invalid filepath for shader {}", m_FilePath);
			m_Compiled = false;
			return;
		}

		String result = FileSystem::ReadFile(m_FilePath);
		auto shaderSources = PreProcess(result);

		for (auto& shaderSource : shaderSources)
			AddGlobalDefines(shaderSource.second);

		if (Compile(shaderSources))
		{
			ATN_CORE_WARN_TAG_("GLShader", "Successfully compile OpenGL Shader '{}'", m_Name);
		}
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

	void GLShader::AddGlobalDefines(String& shaderSource)
	{
		const char* token = "#version";
		uint64 pos = shaderSource.find(token, 0);

		if (pos == String::npos)
			return;

		uint64 eol = shaderSource.find_first_of("\r\n", pos);
		shaderSource.insert(eol, Renderer::GetGlobalShaderMacroses());
	}


	GLIncludeShader::GLIncludeShader(const FilePath& path)
	{
		m_FilePath = path;
		Reload();
	}

	GLIncludeShader::~GLIncludeShader()
	{
		if(m_IsLoaded)
			glDeleteNamedStringARB(m_Name.size(), m_Name.c_str());
	}

	void GLIncludeShader::Reload()
	{
		if (!FileSystem::Exists(m_FilePath))
		{
			ATN_CORE_ERROR_TAG_("GLIncludeShader", "Invalid filepath for shader '{}'", m_FilePath);
			m_IsLoaded = false;
			return;
		}

		if(m_IsLoaded)
			glDeleteNamedStringARB(m_Name.size(), m_Name.c_str());

		String source = FileSystem::ReadFile(m_FilePath);
		m_Name = "/" + m_FilePath.filename().string();

		glNamedStringARB(GL_SHADER_INCLUDE_ARB, m_Name.size(), m_Name.c_str(), source.size(), source.c_str());
		m_IsLoaded = true;
	}
}
