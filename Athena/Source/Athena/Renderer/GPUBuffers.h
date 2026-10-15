#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Buffer.h"
#include "Athena/Renderer/Format.h"

namespace Athena
{
	struct VertexElement
	{
		String Name;
		Format ElementFormat;
		uint32 Offset;

		VertexElement() = default;
		VertexElement(Format format, const String& name)
			: Name(name), ElementFormat(format), Offset(0) {
		}
	};


	class ATHENA_API VertexMemoryLayout
	{
	public:
		using iterator = std::vector<VertexElement>::iterator;
		using const_iterator = std::vector<VertexElement>::const_iterator;

	public:
		VertexMemoryLayout() = default;

		VertexMemoryLayout(const std::initializer_list<VertexElement>& elements)
			: m_Elements(elements)
		{
			CalculateOffsetsAndStride();
		}

		VertexMemoryLayout(const std::vector<VertexElement>& elements)
			: m_Elements(elements)
		{
			CalculateOffsetsAndStride();
		}

		uint32 GetStride() const { return m_Stride; }

		const std::vector<VertexElement>& GetElements() const { return m_Elements; }
		uint32 GetElementsNum() const { return m_Elements.size(); }

		iterator	   begin() { return m_Elements.begin(); }
		const_iterator begin() const { return m_Elements.begin(); }
		iterator	   end() { return m_Elements.end(); }
		const_iterator end()   const { return m_Elements.end(); }

	private:
		void CalculateOffsetsAndStride();

	private:
		std::vector<VertexElement> m_Elements;
		uint32 m_Stride = 0;
	};

	class ATHENA_API VertexBuffer : public RefCounted
	{
	public:
		static Ref<VertexBuffer> Create(const String& name, uint64 size, Buffer* vertices = nullptr);
		virtual ~VertexBuffer() = default;

		virtual void Init() = 0;
		virtual void Destroy() = 0;

		virtual void UploadData(const Buffer& buffer, uint64 offset = 0) = 0;
		virtual void Resize(uint64 size);

		const String& GetName() const { return m_Name; }
		uint64 GetSize() const { return m_Size; };

	private:
		String m_Name;
		uint64 m_Size;
	};

	enum class IndexSize
	{
		UInt32,
		UInt16
	};

	class ATHENA_API IndexBuffer : public RefCounted
	{
	public:
		static Ref<IndexBuffer> Create(const String& name, uint64 size, Buffer* indices = nullptr, IndexSize indexSize = IndexSize::UInt32);
		virtual ~IndexBuffer() = default;

		virtual void Init() = 0;
		virtual void Destroy() = 0;

		virtual void UploadData(const Buffer& buffer, uint64 offset = 0) = 0;
		virtual void Resize(uint64 size);

		const String& GetName() const { return m_Name; }
		uint64 GetSize() const { return m_Size; };
		IndexSize GetIndexSize() const { return m_IndexSize; }
		uint64 GetCount() const;

	private:
		String m_Name;
		uint64 m_Size;
		IndexSize m_IndexSize;
	};

}

