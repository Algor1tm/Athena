#pragma once

#include "Athena/Core/Core.h"


namespace Athena
{
	struct PermutationVector
	{
		std::unordered_map<String, int64> Vector;
	};

	struct PermutationParameter
	{
		String Name;
		std::vector<int64> Values;
		int64 DefaultValue;
	};

	struct PermutationDomain
	{
		std::vector<PermutationParameter> Parameters;
	};

	class PermutationIterator
	{
	public:
		NON_COPYABLE(PermutationIterator);

		PermutationIterator(const PermutationDomain& domain)
			: m_Domain(domain)
			, m_ValueIndices(domain.Parameters.size())
			, m_ReachedEnd(false) // at least 1 permutation
		{
			m_VectorResult.Vector.reserve(m_ValueIndices.size());

			for (uint32 i = 0; i < m_ValueIndices.size(); ++i)
			{
				const PermutationParameter& param = m_Domain.Parameters[i];
				m_VectorResult.Vector[param.Name] = param.Values[m_ValueIndices[i]];
			}
		}
		
		const PermutationVector& GetVector() const
		{
			return m_VectorResult;
		}

		PermutationIterator& operator++()
		{
			if (!*this)
			{
				ensuref(false);
				return *this;
			}

			for (uint32 i = 0; i < m_ValueIndices.size(); ++i)
			{
				++m_ValueIndices[i];

				if (m_ValueIndices[i] < m_Domain.Parameters[i].Values.size())
				{
					OnValueIndexUpdate(i);
					return *this;
				}

				m_ValueIndices[i] = 0;
				OnValueIndexUpdate(i);
			}

			m_ReachedEnd = true;
			return *this;
		}

		operator bool() const
		{
			return !m_ReachedEnd;
		}

		static int64 GetPermutationIndex(const PermutationDomain& domain, const PermutationVector& vector)
		{
			int64 index = 0;
			int64 multiplier = 1;

			for (const PermutationParameter& param : domain.Parameters)
			{
				auto it = vector.Vector.find(param.Name);
				if (it == vector.Vector.end())
					return -1;

				auto valueIt = std::find(param.Values.begin(), param.Values.end(), it->second);

				if (valueIt == param.Values.end())
					return -1;

				int64 valueIndex = std::distance(param.Values.begin(), valueIt);

				index += valueIndex * multiplier;
				multiplier *= param.Values.size();
			}

			return index;
		}

	private:
		void OnValueIndexUpdate(uint32 parameterIndex)
		{
			const PermutationParameter& param = m_Domain.Parameters[parameterIndex];
			m_VectorResult.Vector[param.Name] = param.Values[m_ValueIndices[parameterIndex]];
		}

	private:
		const PermutationDomain& m_Domain;
		PermutationVector m_VectorResult;
		std::vector<uint32> m_ValueIndices;
		bool m_ReachedEnd;
	};
}
