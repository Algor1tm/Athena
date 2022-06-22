#include "atnpch.h"
#include "OpenGLVertexArray.h"

#include "glad/glad.h"


namespace Athena
{
	static GLenum ShaderDataTypeToOpenGLBaseType(ShaderDataType type)
	{
		switch (type)
		{
			case ShaderDataType::Float:  return GL_FLOAT;
			case ShaderDataType::Float2: return GL_FLOAT;
			case ShaderDataType::Float3: return GL_FLOAT;
			case ShaderDataType::Float4: return GL_FLOAT;
			case ShaderDataType::Mat3:	 return GL_FLOAT;
			case ShaderDataType::Mat4:   return GL_FLOAT;
			case ShaderDataType::Int:    return GL_INT;
			case ShaderDataType::Int2:   return GL_INT;
			case ShaderDataType::Int3:   return GL_INT;
			case ShaderDataType::Int4:   return GL_INT;
			case ShaderDataType::Bool:   return GL_BOOL;
		}

		ATN_CORE_ASSERT(false, "Unknown ShaderDataType!");
		return 0;
	}


	OpenGLVertexArray::OpenGLVertexArray()
	{
		ATN_PROFILE_FUNCTION();

		glCreateVertexArrays(0, &m_RendererID);
	}

	OpenGLVertexArray::~OpenGLVertexArray()
	{
		ATN_PROFILE_FUNCTION();

		glDeleteVertexArrays(1, &m_RendererID);
	}

	void OpenGLVertexArray::Bind() const
	{
		ATN_PROFILE_FUNCTION();

		glBindVertexArray(m_RendererID);
	}

	void OpenGLVertexArray::UnBind() const
	{
		ATN_PROFILE_FUNCTION();

		glBindVertexArray(0);
	}

	void OpenGLVertexArray::AddVertexBuffer(const Ref<VertexBuffer>& vertexBuffer)
	{
		ATN_PROFILE_FUNCTION();

		glBindVertexArray(m_RendererID);
		vertexBuffer->Bind();

		const auto& layout = vertexBuffer->GetLayout();
		ATN_CORE_ASSERT(layout.GetElements().size(), "Vertex Buffer has no layout");


		uint32_t index = 0;
		for (const auto& elem : layout)
		{
			glEnableVertexAttribArray(index);
			glVertexAttribPointer(index,
				elem.GetComponentCount(),
				ShaderDataTypeToOpenGLBaseType(elem.Type),
				elem.Normalized ? GL_TRUE : GL_FALSE,
				layout.GetStride(),
				reinterpret_cast<const void*>((uint64_t)elem.Offset));

			++index;
		}
		m_VertexBuffers.push_back(vertexBuffer);
	}

	void OpenGLVertexArray::SetIndexBuffer(const Ref<IndexBuffer>& indexBuffer)
	{
		ATN_PROFILE_FUNCTION();

		glBindVertexArray(m_RendererID);
		indexBuffer->Bind();

		m_IndexBuffer = indexBuffer;
	}
}
