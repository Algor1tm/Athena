#include "atnpch.h"
#include "GLBuffer.h"

#include <glad/glad.h>


namespace Athena
{
	static GLenum ShaderDataTypeToGLBaseType(ShaderDataType type)
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


	///////////////////// VertexBuffer /////////////////////

	GLVertexBuffer::GLVertexBuffer(const VertexBufferDescription& desc)
	{
		glCreateVertexArrays(1, &m_VertexArrayRendererID);

		glCreateBuffers(1, &m_RendererID);
		glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
		glBufferData(GL_ARRAY_BUFFER, desc.Size, desc.Data, desc.BufferUsage == Usage::STATIC ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW);

		SetLayout((*desc.pBufferLayout));
		SetIndexBuffer(desc.pIndexBuffer);
	}

	GLVertexBuffer::~GLVertexBuffer()
	{
		glDeleteVertexArrays(1, &m_VertexArrayRendererID);
		glDeleteBuffers(1, &m_RendererID);
	}

	void GLVertexBuffer::Bind() const
	{
		glBindVertexArray(m_VertexArrayRendererID);
	}

	void GLVertexBuffer::UnBind() const
	{
		glBindVertexArray(0);
	}

	void GLVertexBuffer::SetData(const void* data, uint32 size)
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
		glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
	}

	void GLVertexBuffer::SetLayout(const BufferLayout& layout)
	{
		ATN_CORE_ASSERT(layout.GetElements().size(), "Invalid buffer layout!");

		glBindVertexArray(m_VertexArrayRendererID);
		glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);

		uint32 vertexBufferIndex = 0;

		for (const auto& element : layout)
		{
			switch (element.Type)
			{
			case ShaderDataType::Float:
			case ShaderDataType::Float2:
			case ShaderDataType::Float3:
			case ShaderDataType::Float4:
			{
				glEnableVertexAttribArray(vertexBufferIndex);
				glVertexAttribPointer(vertexBufferIndex,
					element.GetComponentCount(),
					ShaderDataTypeToGLBaseType(element.Type),
					element.Normalized ? GL_TRUE : GL_FALSE,
					layout.GetStride(),
					reinterpret_cast<const void*>((uint64)element.Offset));

				vertexBufferIndex++;
				break;
			}
			case ShaderDataType::Int:
			case ShaderDataType::Int2:
			case ShaderDataType::Int3:
			case ShaderDataType::Int4:
			case ShaderDataType::Bool:
			{
				glEnableVertexAttribArray(vertexBufferIndex);
				glVertexAttribIPointer(vertexBufferIndex,
					element.GetComponentCount(),
					ShaderDataTypeToGLBaseType(element.Type),
					layout.GetStride(),
					reinterpret_cast<const void*>((uint64)element.Offset));

				vertexBufferIndex++;
				break;
			}
			case ShaderDataType::Mat3:
			case ShaderDataType::Mat4:
			{
				uint8_t count = element.GetComponentCount();
				for (uint8_t i = 0; i < count; i++)
				{
					glEnableVertexAttribArray(vertexBufferIndex);
					glVertexAttribPointer(vertexBufferIndex,
						count,
						ShaderDataTypeToGLBaseType(element.Type),
						element.Normalized ? GL_TRUE : GL_FALSE,
						layout.GetStride(),
						reinterpret_cast<const void*>((uint64)element.Offset + sizeof(float) * count * i));
					glVertexAttribDivisor(vertexBufferIndex, 1);
					vertexBufferIndex++;
				}
				break;
			}
			default:
				ATN_CORE_ASSERT(false, "Unknown ShaderDataType!");
			}

		}
	}

	void GLVertexBuffer::SetIndexBuffer(const Ref<IndexBuffer>& indexBuffer)
	{
		glBindVertexArray(m_VertexArrayRendererID);
		indexBuffer->Bind();

		m_IndexBuffer = indexBuffer;
	}

	///////////////////// IndexBuffer /////////////////////

	GLIndexBuffer::GLIndexBuffer(uint32* indices, uint32 count)
		: m_Count(count)
	{
		glCreateBuffers(1, &m_RendererID);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(uint32), indices, GL_STATIC_DRAW);
	}

	GLIndexBuffer::~GLIndexBuffer()
	{
		glDeleteBuffers(1, &m_RendererID);
	}

	void GLIndexBuffer::Bind() const
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID);
	}

	void GLIndexBuffer::UnBind() const
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
}
