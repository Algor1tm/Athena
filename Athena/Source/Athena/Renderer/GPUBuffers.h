#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Log.h"
#include "Athena/Renderer/ShaderResource.h"


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


	struct VertexBufferElement
	{
		String Name;
		ShaderDataType Type;
		uint32 Size;
		uint32 Offset;
		bool Normalized;
		uint32 Location;

		VertexBufferElement() = default;
		VertexBufferElement(ShaderDataType type, const String& name, uint32 location ,bool normalized = false)
			: Name(name), Type(type), Location(location), Size(ShaderDataTypeSize(type)), Offset(0), Normalized(normalized) {}

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


	class ATHENA_API VertexBufferLayout
	{
	public:
		using iterator = std::vector<VertexBufferElement>::iterator;
		using const_iterator = std::vector<VertexBufferElement>::const_iterator;

	public:
		VertexBufferLayout() = default;

		VertexBufferLayout(const std::initializer_list<VertexBufferElement>& elements)
			: m_Elements(elements)
		{
			CalculateOffsetsAndStride();
		}

		VertexBufferLayout(const std::vector<VertexBufferElement>& elements)
			: m_Elements(elements) 
		{
			CalculateOffsetsAndStride();
		}

		VertexBufferLayout(std::vector<VertexBufferElement>&& elements)
			: m_Elements(std::move(elements))
		{
			CalculateOffsetsAndStride();
		}

		uint32 GetStride() const { return m_Stride; }

		const std::vector<VertexBufferElement>& GetElements() const { return m_Elements; }
		uint32 GetElementsNum() const { return m_Elements.size(); }

		iterator	   begin()	     { return m_Elements.begin(); }
		const_iterator begin() const { return m_Elements.begin(); }
		iterator	   end()		 { return m_Elements.end(); }
		const_iterator end()   const { return m_Elements.end(); }

	private:
		void CalculateOffsetsAndStride()
		{
			std::vector<VertexBufferElement> sortedElems(m_Elements.size());
			for (auto& elem : m_Elements)
				sortedElems[elem.Location] = elem;

			m_Elements = sortedElems;

			uint32 offset = 0;
			for (auto& elem: m_Elements)
			{
				elem.Offset = offset;
				offset += elem.Size;
				m_Stride += elem.Size;
			}
		}

	private:
		std::vector<VertexBufferElement> m_Elements;
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
		const void* Data;
		uint32 Count;
		BufferUsage Usage;
	};

	class ATHENA_API IndexBuffer : public RefCounted
	{
	public:
		static Ref<IndexBuffer> Create(const IndexBufferCreateInfo& info);
		virtual ~IndexBuffer() = default;

		virtual void RT_SetData(const void* data, uint64 size, uint64 offset = 0) = 0;

		uint32 GetCount() const { return m_Info.Count; }
		const IndexBufferCreateInfo& GetInfo() const { return m_Info; }

	protected:
		IndexBufferCreateInfo m_Info;
	};

	struct VertexBufferCreateInfo
	{
		String Name;
		const void* Data;
		uint64 Size;
		Ref<IndexBuffer> IndexBuffer;
		BufferUsage Usage;
	};

	class ATHENA_API VertexBuffer : public RefCounted
	{
	public:
		static Ref<VertexBuffer> Create(const VertexBufferCreateInfo& info);
		virtual ~VertexBuffer() = default;

		virtual void RT_SetData(const void* data, uint64 size, uint64 offset = 0) = 0;

		uint64 GetSize() const { return m_Info.Size; }
		Ref<IndexBuffer> GetIndexBuffer() const { return m_Info.IndexBuffer; }
		const VertexBufferCreateInfo& GetInfo() const { return m_Info; }

	protected:
		VertexBufferCreateInfo m_Info;
	};


	class ATHENA_API UniformBuffer : public ShaderResource
	{
	public:
		static Ref<UniformBuffer> Create(const String& name, uint64 size);
		virtual ~UniformBuffer() = default;

		virtual ShaderResourceType GetResourceType() override { return ShaderResourceType::UniformBuffer; }

		virtual uint64 GetSize() = 0;
		virtual void RT_SetData(const void* data, uint64 size, uint64 offset = 0) = 0;
	};


	class ATHENA_API StorageBuffer : public ShaderResource
	{
	public:
		static Ref<StorageBuffer> Create(const String& name, uint64 size);
		virtual ~StorageBuffer() = default;

		virtual ShaderResourceType GetResourceType() override { return ShaderResourceType::StorageBuffer; }

		virtual uint64 GetSize() = 0;
		virtual void RT_SetData(const void* data, uint64 size, uint64 offset = 0) = 0;
	};
}
