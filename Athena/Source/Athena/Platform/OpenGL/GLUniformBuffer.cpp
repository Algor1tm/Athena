#include "GLUniformBuffer.h"

#include <glad/glad.h>


namespace Athena
{
	GLUniformBuffer::GLUniformBuffer(uint32 size, uint32 binding)
	{
		glCreateBuffers(1, &m_RendererID);
		glNamedBufferData(m_RendererID, size, nullptr, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, binding, m_RendererID);
	}

	GLUniformBuffer::~GLUniformBuffer()
	{
		glDeleteBuffers(1, &m_RendererID);
	}

	void GLUniformBuffer::SetData(const void* data, uint32 size, uint32 offset)
	{
		glNamedBufferSubData(m_RendererID, offset, size, data);
	}
}
