#pragma once

#include "Athena/Core/Core.h"

#include <unordered_map>


namespace Athena
{
	enum class ScriptFieldType
	{
		None = 0,
		Bool, Int32, Float,
		Vector2, Vector3, Vector4,
	};

	// Default case to trigger a compile-time error for unsupported types
	template <typename T>
	struct ScriptFieldTypeMapper
	{
		static_assert(!std::is_same_v<T, T>, "Not supported type for script field!");
	};

	template <>
	struct ScriptFieldTypeMapper<bool>    { static constexpr ScriptFieldType Value = ScriptFieldType::Bool; };

	template <>
	struct ScriptFieldTypeMapper<int32>   { static constexpr ScriptFieldType Value = ScriptFieldType::Int32; };

	template <>
	struct ScriptFieldTypeMapper<float>   { static constexpr ScriptFieldType Value = ScriptFieldType::Float; };

	template <>
	struct ScriptFieldTypeMapper<Vector2> { static constexpr ScriptFieldType Value = ScriptFieldType::Vector2; };

	template <>
	struct ScriptFieldTypeMapper<Vector3> { static constexpr ScriptFieldType Value = ScriptFieldType::Vector3; };

	template <>
	struct ScriptFieldTypeMapper<Vector4> { static constexpr ScriptFieldType Value = ScriptFieldType::Vector4; };


	template <typename T>
	inline constexpr ScriptFieldType ScriptFieldTypeMapper_V = ScriptFieldTypeMapper<T>::Value;


	class ScriptFieldStorage
	{
	public:
		friend class ScriptInstance;
		friend class ScriptClass;

		ScriptFieldStorage()
		{
			memset(m_Buffer, 0, sizeof(m_Buffer));
		}

		template <typename T>
		ScriptFieldStorage(T* ref)
		{
			m_InternalRef = ref;

			m_Type = ScriptFieldTypeMapper_V<T>;
			m_Size = GetScriptFieldSize(m_Type);
			memcpy(m_Buffer, ref, m_Size);
		}

		ScriptFieldStorage(const ScriptFieldStorage& other)
		{
			m_Type = other.m_Type;
			m_InternalRef = other.m_InternalRef;
			m_Size = other.m_Size;
			memcpy(m_Buffer, other.m_Buffer, sizeof(m_Buffer));
		}

		ScriptFieldStorage& operator=(const ScriptFieldStorage& other)
		{
			m_Type = other.m_Type;
			m_InternalRef = other.m_InternalRef;
			m_Size = other.m_Size;
			memcpy(m_Buffer, other.m_Buffer, sizeof(m_Buffer));
			return *this;
		}

		template<typename T>
		T GetValue()
		{
			static_assert(sizeof(T) <= 16, "Type too large!");

			if (m_InternalRef != nullptr)
				memcpy(m_Buffer, m_InternalRef, m_Size);

			return *(T*)m_Buffer;
		}

		template<typename T>
		void SetValue(T value)
		{
			static_assert(sizeof(T) <= 16, "Type too large!");
			memcpy(m_Buffer, &value, sizeof(T));

			if (m_InternalRef != nullptr)
				memcpy(m_InternalRef, m_Buffer, m_Size);
		}

		ScriptFieldType GetType() const
		{
			return m_Type;
		}

		const void* GetBuffer() const
		{
			return m_Buffer;
		}

		void SetInternalRef(void* ref)
		{
			m_InternalRef = ref;

			if(ref != nullptr)
				memcpy(m_InternalRef, m_Buffer, m_Size);
		}

		void* GetInternalRef()
		{
			return m_InternalRef;
		}

	private:
		uint32 GetScriptFieldSize(ScriptFieldType type)
		{
			switch (type)
			{
			case ScriptFieldType::Bool:  return 1;
			case ScriptFieldType::Int32: return 4;
			case ScriptFieldType::Float: return 4;

			case ScriptFieldType::Vector2: return 8;
			case ScriptFieldType::Vector3: return 12;
			case ScriptFieldType::Vector4: return 16;
			}

			ATN_CORE_ASSERT(false);
			return 0;
		}

	private:
		ScriptFieldType m_Type = ScriptFieldType::None;
		void* m_InternalRef = nullptr;
		uint32 m_Size = 0;
		byte m_Buffer[16];
	};

	using ScriptFieldMap = std::unordered_map<std::string_view, ScriptFieldStorage>;
}
