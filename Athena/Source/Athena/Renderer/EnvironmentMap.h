#pragma once

#include "Athena/Core/Core.h"


namespace Athena
{
	class ATHENA_API Cubemap;

	class ATHENA_API EnvironmentMap
	{
	public:
		static Ref<EnvironmentMap> Create(const FilePath& hdrMap);

		void Bind();
		const FilePath& GetFilePath() { return m_FilePath; };

	private:
		Ref<Cubemap> m_PrefilteredMap;
		Ref<Cubemap> m_IrradianceMap;

		FilePath m_FilePath;
	};
}
