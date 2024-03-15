#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Log.h"
#include "Athena/Renderer/RenderResource.h"


namespace Athena
{
	enum class ShaderDataType : uint8
	{
		Unknown = 0, Float, Float2, Float3, Float4, Int, Int2, Int3, Int4, UInt, Mat3, Mat4
	};

	constexpr uint32 ShaderDataTypeSize(ShaderDataType type)
	{
		switch (type)
		{
			case ShaderDataType::Float:  return 4;
			case ShaderDataType::Float2: return 4 * 2;
			case ShaderDataType::Float3: return 4 * 3;
			case ShaderDataType::Float4: return 4 * 4;
			case ShaderDataType::Int:    return 4;
			case ShaderDataType::Int2:   return 4 * 2;
			case ShaderDataType::Int3:   return 4 * 3;
			case ShaderDataType::Int4:   return 4 * 4;
			case ShaderDataType::UInt:   return 4;
			case ShaderDataType::Mat3:   return 4 * 9;
			case ShaderDataType::Mat4:   return 4 * 16;
		}

		ATN_CORE_ASSERT(false, "Unknown ShaderDataType!");
		return 0;
	}

	constexpr std::string_view ShaderDataTypeToString(ShaderDataType type)
	{
		switch (type)
		{
		case ShaderDataType::Float:  return "float";
		case ShaderDataType::Float2: return "float2";
		case ShaderDataType::Float3: return "float3";
		case ShaderDataType::Float4: return "float4";
		case ShaderDataType::Int:    return "int";
		case ShaderDataType::Int2:   return "int2";
		case ShaderDataType::Int3:   return "int3";
		case ShaderDataType::Int4:   return "int4";
		case ShaderDataType::UInt:   return "uint";
		case ShaderDataType::Mat3:   return "mat3";
		case ShaderDataType::Mat4:   return "mat4";
		}

		ATN_CORE_ASSERT(false, "Unknown ShaderDataType!");
		return "";
	}


	struct VertexElement
	{
		String Name;
		ShaderDataType Type;
		uint32 Size;
		uint32 Offset;

		VertexElement() = default;
		VertexElement(ShaderDataType type, const String& name)
			: Name(name), Type(type), Size(ShaderDataTypeSize(type)), Offset(0) {}

		uint32 GetComponentCount() const
		{
			switch (Type)
			{
				case ShaderDataType::Float: return 1;
				case ShaderDataType::Float2: return 2;
				case ShaderDataType::Float3: return 3;
				case ShaderDataType::Float4: return 4;
				case ShaderDataType::Mat3: return 9;
				case ShaderDataType::Mat4: return 16;
				case ShaderDataType::Int: return 1;
				case ShaderDataType::Int2: return 2;
				case ShaderDataType::Int3: return 3;
				case ShaderDataType::Int4: return 4;
				case ShaderDataType::UInt: return 4;
			}

			ATN_CORE_ASSERT(false, "Unknown ShaderDataType!");
			return 0;
		}
	};


	class ATHENA_API VertexLayout
	{
	public:
		using iterator = std::vector<VertexElement>::iterator;
		using const_iterator = std::vector<VertexElement>::const_iterator;

	public:
		VertexLayout() = default;

		VertexLayout(const std::initializer_list<VertexElement>& elements)
			: m_Elements(elements)
		{
			CalculateOffsetsAndStride();
		}

		VertexLayout(const std::vector<VertexElement>& elements)
			: m_Elements(elements) 
		{
			CalculateOffsetsAndStride();
		}

		uint32 GetStride() const { return m_Stride; }

		const std::vector<VertexElement>& GetElements() const { return m_Elements; }
		uint32 GetElementsNum() const { return m_Elements.size(); }

		iterator	   begin()	     { return m_Elements.begin(); }
		const_iterator begin() const { return m_Elements.begin(); }
		iterator	   end()		 { return m_Elements.end(); }
		const_iterator end()   const { return m_Elements.end(); }

	private:
		void CalculateOffsetsAndStride()
		{
			uint32 offset = 0;
			for (auto& elem: m_Elements)
			{
				elem.Offset = offset;
				offset += elem.Size;
				m_Stride += elem.Size;
			}
		}

	private:
		std::vector<VertexElement> m_Elements;
		uint32 m_Stride = 0;
	};

	
	enum class BufferUsage
	{
		STATIC,
		DYNAMIC
	};

	struct IndexBufferCreateInfo
	{
		String Name;
		const void* Data = nullptr;
		uint32 Count = 0;
		BufferUsage Usage = BufferUsage::STATIC;
	};

	class ATHENA_API IndexBuffer : public RefCounted
	{
	public:
		static Ref<IndexBuffer> Create(const IndexBufferCreateInfo& info);
		virtual ~IndexBuffer() = default;

		virtual void UploadData(const void* data, uint64 size, uint64 offset = 0) = 0;

		uint32 GetCount() const { return m_Info.Count; }
		const IndexBufferCreateInfo& GetInfo() const { return m_Info; }

	protected:
		IndexBufferCreateInfo m_Info;
	};

	struct VertexBufferCreateInfo
	{
		String Name;
		const void* Data = nullptr;
		uint64 Size = 0;
		Ref<IndexBuffer> IndexBuffer;
		BufferUsage Usage = BufferUsage::STATIC;
	};

	class ATHENA_API VertexBuffer : public RefCounted
	{
	public:
		static Ref<VertexBuffer> Create(const VertexBufferCreateInfo& info);
		virtual ~VertexBuffer() = default;

		virtual void UploadData(const void* data, uint64 size, uint64 offset = 0) = 0;

		uint64 GetSize() const { return m_Info.Size; }

		Ref<IndexBuffer> GetIndexBuffer() const { return m_Info.IndexBuffer; }
		const VertexBufferCreateInfo& GetInfo() const { return m_Info; }

	protected:
		VertexBufferCreateInfo m_Info;
	};


	class ATHENA_API UniformBuffer : public RenderResource
	{
	public:
		static Ref<UniformBuffer> Create(const String& name, uint64 size);
		virtual ~UniformBuffer() = default;

		virtual RenderResourceType GetResourceType() override { return RenderResourceType::UniformBuffer; }

		virtual uint64 GetSize() = 0;
		virtual void UploadData(const void* data, uint64 size, uint64 offset = 0) = 0;
	};


	class ATHENA_API StorageBuffer : public RenderResource
	{
	public:
		static Ref<StorageBuffer> Create(const String& name, uint64 size);
		virtual ~StorageBuffer() = default;

		virtual RenderResourceType GetResourceType() override { return RenderResourceType::StorageBuffer; }

		virtual uint64 GetSize() = 0;
		virtual void UploadData(const void* data, uint64 size, uint64 offset = 0) = 0;
	};
}
