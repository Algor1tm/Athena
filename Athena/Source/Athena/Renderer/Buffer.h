#pragma once


namespace Athena
{
	enum class ShaderDataType : uint8
	{
		None = 0, Float, Float2, Float3, Float4, Mat3, Mat4, Int, Int2, Int3, Int4, Bool
	};

	constexpr uint32 ShaderDataTypeSize(ShaderDataType type)
	{
		switch (type)
		{
			case ShaderDataType::Float:  return 4;
			case ShaderDataType::Float2: return 4 * 2;
			case ShaderDataType::Float3: return 4 * 3;
			case ShaderDataType::Float4: return 4 * 4;
			case ShaderDataType::Mat3:   return 4 * 9;
			case ShaderDataType::Mat4:   return 4 * 16;
			case ShaderDataType::Int:    return 4;
			case ShaderDataType::Int2:   return 4 * 2;
			case ShaderDataType::Int3:   return 4 * 3;
			case ShaderDataType::Int4:   return 4 * 4;
			case ShaderDataType::Bool:   return 1;
		}

		ATN_CORE_ASSERT(false, "Unknown ShaderDataType!");
		return 0;
	}


	struct ATHENA_API BufferElement
	{
		String Name;
		ShaderDataType Type;
		uint32 Size;
		uint32 Offset;
		bool Normalized;

		BufferElement() = default;
		BufferElement(ShaderDataType type, const String& name, bool normalized = false)
			: Name(name), Type(type), Size(ShaderDataTypeSize(type)), Offset(0), Normalized(normalized) {}

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
				case ShaderDataType::Bool: return 1;
			}

			ATN_CORE_ASSERT(false, "Unknown ShaderDataType!");
			return 0;
		}
	};


	class ATHENA_API BufferLayout
	{
	public:
		using iterator = std::vector<BufferElement>::iterator;
		using const_iterator = std::vector<BufferElement>::const_iterator;

	public:
		BufferLayout() = default;
		BufferLayout(const std::initializer_list<BufferElement>& elements)
			: m_Elements(elements) 
		{
			CalculateOffsetAndStride();
		}

		inline uint32 GetStride() const { return m_Stride; }
		inline const std::vector<BufferElement>& GetElements() const { return m_Elements; }

		inline iterator		  begin()	    { return m_Elements.begin(); }
		inline const_iterator begin() const { return m_Elements.begin(); }
		inline iterator		  end()		    { return m_Elements.end(); }
		inline const_iterator end()   const { return m_Elements.end(); }

	private:
		void CalculateOffsetAndStride()
		{
			uint32 stride = 0;
			uint32 offset = 0;

			for (auto& elem : m_Elements)
			{
				elem.Offset = offset;
				offset += elem.Size;
				m_Stride += elem.Size;
			}
		}

	private:
		std::vector<BufferElement> m_Elements;
		uint32 m_Stride = 0;
	};

	
	class ATHENA_API VertexBuffer
	{
	public:
		virtual ~VertexBuffer() = default;

		virtual void Bind() const = 0;
		virtual void UnBind() const = 0;

		virtual void SetLayout(const BufferLayout& layout) = 0;
		virtual const BufferLayout& GetLayout() const = 0;

		virtual void SetData(const void* data, uint32 size) = 0;

		static Ref<VertexBuffer> Create(float* vertices, uint32 size);
		static Ref<VertexBuffer> Create(uint32 size);
	};

	class ATHENA_API IndexBuffer
	{
	public:
		virtual ~IndexBuffer() = default;

		virtual void Bind() const = 0;
		virtual void UnBind() const = 0;

		virtual uint32 GetCount() const = 0;

		static Ref<IndexBuffer> Create(uint32* indices, uint32 count);
	};
}
