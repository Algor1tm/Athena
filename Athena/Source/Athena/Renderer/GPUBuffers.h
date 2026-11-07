#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Buffer.h"
#include "Athena/Renderer/Format.h"
#include "Athena/Renderer/ResourceState.h"


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

	enum class GPUBufferType
	{
		VertexBuffer,
		IndexBuffer,
		StructuredBuffer,
	};

	enum GPUBufferFlags
	{
		GBF_None = BIT(0),
		GBF_AllowUAV = BIT(1)
	};

	struct GPUBufferCreateInfo
	{
		String Name;
		GPUBufferType Type = GPUBufferType::VertexBuffer;
		GPUBufferFlags Flags = GBF_None;
		uint64 Size = 0;
		uint32 Stride = 0;
	};

	class ATHENA_API GPUBuffer: public RefCounted
	{
	public:
		static Ref<GPUBuffer> Create(const GPUBufferCreateInfo& info, Buffer* data = nullptr);

		GPUBuffer(const GPUBufferCreateInfo& info);
		virtual ~GPUBuffer() = default;

		virtual void Init() = 0;

		ResourceState GetState() const;
		void SetState(ResourceState state);

		const GPUBufferCreateInfo& GetInfo() const { return m_Info; }
		const String& GetName() const { return m_Info.Name; }
		GPUBufferType GetType() const { return m_Info.Type; }
		uint64 GetSize() const { return m_Info.Size; };
		uint32 GetStride() const { return m_Info.Stride; }
		uint64 GetCount() const { return m_Info.Size / m_Info.Stride; }
		GPUBufferFlags GetFlags() const { return m_Info.Flags; }

	private:
		GPUBufferCreateInfo m_Info;
		ResourceState m_State;
	};

	using VertexBuffer = GPUBuffer;
	using IndexBuffer = GPUBuffer;
	using StructuredBuffer = GPUBuffer;
}
