#include "GLShader.h"

#include "Athena/Core/FileSystem.h"

#include <glad/glad.h>


namespace Athena
{
	static GLenum ShaderTypeToGLenum(ShaderType type)
	{
		switch (type)
		{
		case ShaderType::VERTEX_SHADER: return GL_VERTEX_SHADER;
		case ShaderType::FRAGMENT_SHADER: return GL_FRAGMENT_SHADER;
		case ShaderType::GEOMETRY_SHADER: return GL_GEOMETRY_SHADER;
		case ShaderType::COMPUTE_SHADER: return GL_COMPUTE_SHADER;
		}

		ATN_CORE_ASSERT(false, "Unknown shader type!");
		return 0;
	}

	static bool CompileShaderSources(const std::unordered_map<ShaderType, String>& shaderSources, const String& name, GLuint* program)
	{
		*program = glCreateProgram();

		std::vector<GLenum> shaderIDs(shaderSources.size(), -1);
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

				for (auto id : shaderIDs)
				{
					if (id != -1)
						glDeleteShader(id);
				}

				ATN_CORE_ERROR("{0} '{1}' compilation failed!", ShaderTypeToString(glType), name);
				ATN_CORE_WARN("Errors/Warnings:\n{}", infoLog.data());

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

			ATN_CORE_ERROR("{0}", infoLog.data());
			ATN_CORE_ASSERT(false, "Program linking failure!");

			return false;
		}

		for (auto id : shaderIDs)
			glDetachShader(*program, id);

		return true;
	}


	GLShader::GLShader(const FilePath& path)
	{
		ATN_CORE_ASSERT(FileSystem::Exists(path), "Invalid filepath for Shader");

		m_FilePath = path;
		m_Name = m_FilePath.stem().string();

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
		m_Compiled = CompileShaderSources(shaderSources, m_Name, &m_RendererID);
		return m_Compiled;
	}

	void GLShader::Reload()
	{
		String result = FileSystem::ReadFile(m_FilePath);
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


	GLIncludeShader::GLIncludeShader(const FilePath& path)
	{
		ATN_CORE_ASSERT(FileSystem::Exists(path), "Invalid filepath for Shader");

		m_FilePath = path;
		Reload();
	}

	GLIncludeShader::~GLIncludeShader()
	{
		glDeleteNamedStringARB(m_Name.size(), m_Name.c_str());
	}

	void GLIncludeShader::Reload()
	{
		String source = FileSystem::ReadFile(m_FilePath);
		m_Name = "/" + m_FilePath.filename().string();

		glNamedStringARB(GL_SHADER_INCLUDE_ARB, m_Name.size(), m_Name.c_str(), source.size(), source.c_str());
	}


	GLComputeShader::GLComputeShader(const FilePath& path, const Vector3i& workGroupSize)
	{
		ATN_CORE_ASSERT(FileSystem::Exists(path), "Invalid filepath for Shader");

		m_WorkGroupSize = workGroupSize;
		m_FilePath = path;
		Reload();
	}

	GLComputeShader::~GLComputeShader()
	{
		glDeleteProgram(m_RendererID);
	}

	void GLComputeShader::Execute(uint32 x, uint32 y, uint32 z)
	{
		if (m_Compiled)
		{
			glDispatchCompute(x / m_WorkGroupSize.x, y / m_WorkGroupSize.y, z / m_WorkGroupSize.z);
			glMemoryBarrier(GL_ALL_BARRIER_BITS);
			//glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_UNIFORM_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT | GL_FRAMEBUFFER_BARRIER_BIT);
		}
	}

	void GLComputeShader::Reload()
	{
		String result = FileSystem::ReadFile(m_FilePath);
		Compile(result);
	}

	void GLComputeShader::Bind() const
	{
		if (m_Compiled)
			glUseProgram(m_RendererID);
		else
			glUseProgram(0);
	}

	void GLComputeShader::UnBind() const
	{
		glUseProgram(0);
	}

	bool GLComputeShader::Compile(const String& sourceString)
	{
		std::unordered_map<ShaderType, String> sources;
		sources[ShaderType::COMPUTE_SHADER] = sourceString;

		m_Compiled = CompileShaderSources(sources, m_Name, &m_RendererID);
		return m_Compiled;
	}
}
