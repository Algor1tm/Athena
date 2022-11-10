#pragma once

#include "Athena/Core/Core.h"

#include <unordered_map>


namespace Athena
{
	enum class ScriptFieldType
	{
		None = 0,
		Int, Float, String, Bool,	// TODO: support string fields
		Vector2, Vector3, Vector4,
	};

	struct ScriptFieldStorage
	{
		friend class ScriptInstance;
		friend class ScriptClass;

		ScriptFieldStorage()
		{
			memset(m_Buffer, 0, sizeof(m_Buffer));
		}

		ScriptFieldStorage(const ScriptFieldStorage& other)
		{
			memcpy(m_Buffer, other.m_Buffer, sizeof(m_Buffer));
		}

		ScriptFieldStorage& operator=(const ScriptFieldStorage& other)
		{
			memcpy(m_Buffer, other.m_Buffer, sizeof(m_Buffer));
			return *this;
		}

		template<typename T>
		T GetValue() const
		{
			static_assert(sizeof(T) <= 16, "Type too large!");
			return *(T*)m_Buffer;
		}

		template<typename T>
		void SetValue(T value)
		{
			static_assert(sizeof(T) <= 16, "Type too large!");
			memcpy(m_Buffer, &value, sizeof(T));
		}

	private:
		byte m_Buffer[16];
	};

	struct ScriptField
	{
		String Name;
		ScriptFieldType Type;
		ScriptFieldStorage Storage;
	};

	using ScriptFieldMap = std::unordered_map<String, ScriptFieldStorage>;
	using ScriptFieldsDescription = std::unordered_map<String, ScriptField>;
}
