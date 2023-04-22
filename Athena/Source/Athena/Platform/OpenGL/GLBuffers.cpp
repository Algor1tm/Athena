#include "GLBuffers.h"

#include "Athena/Platform/OpenGL/GLUtils.h"

#include <glad/glad.h>


namespace Athena
{
	/////////////////////////////////////////////////////////
	///////////////////// VertexBuffer /////////////////////
	/////////////////////////////////////////////////////////

	GLVertexBuffer::GLVertexBuffer(const VertexBufferDescription& desc)
	{
		glCreateVertexArrays(1, &m_VertexArrayRendererID);

		glCreateBuffers(1, &m_RendererID);
		glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
		glBufferData(GL_ARRAY_BUFFER, desc.Size, desc.Data, desc.Usage == BufferUsage::STATIC ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW);

		SetLayout(desc.Layout);
		SetIndexBuffer(desc.IndexBuffer);

		glBindVertexArray(0);
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
					Utils::ShaderDataTypeToGLBaseType(element.Type),
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
					Utils::ShaderDataTypeToGLBaseType(element.Type),
					layout.GetStride(),
					reinterpret_cast<const void*>((uint64)element.Offset));

				vertexBufferIndex++;
				break;
			}
			case ShaderDataType::Mat3:
			case ShaderDataType::Mat4:
			{
				uint8 count = element.GetComponentCount();
				for (uint8 i = 0; i < count; i++)
				{
					glEnableVertexAttribArray(vertexBufferIndex);
					glVertexAttribPointer(vertexBufferIndex,
						count,
						Utils::ShaderDataTypeToGLBaseType(element.Type),
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

	/////////////////////////////////////////////////////////
	///////////////////// IndexBuffer ///////////////////////
	/////////////////////////////////////////////////////////

	GLIndexBuffer::GLIndexBuffer(uint32* indices, uint32 count)
		: m_Count(count)
	{
		glBindVertexArray(0);

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

	/////////////////////////////////////////////////////////
	///////////////////// UniformBuffer /////////////////////
	/////////////////////////////////////////////////////////

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

	/////////////////////////////////////////////////////////
	///////////////////// ShaderStorageBuffer ///////////////
	/////////////////////////////////////////////////////////

	GLShaderStorageBuffer::GLShaderStorageBuffer(uint32 size, uint32 binding)
	{
		glCreateBuffers(1, &m_RendererID);
		glNamedBufferData(m_RendererID, size, nullptr, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, m_RendererID);
	}

	GLShaderStorageBuffer::~GLShaderStorageBuffer()
	{
		glDeleteBuffers(1, &m_RendererID);
	}

	void GLShaderStorageBuffer::SetData(const void* data, uint32 size, uint32 offset)
	{
		glNamedBufferSubData(m_RendererID, offset, size, data);
	}
}
