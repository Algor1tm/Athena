#pragma once

#include "Athena/Core/Core.h"


namespace Athena
{
	class ATHENA_API TextureCube;

	class ATHENA_API EnvironmentMap
	{
	public:
		static Ref<EnvironmentMap> Create(const FilePath& hdrMap);

		void Bind();
		const FilePath& GetFilePath() { return m_FilePath; };

	private:
		Ref<TextureCube> m_PrefilteredMap;
		Ref<TextureCube> m_IrradianceMap;

		FilePath m_FilePath;
	};
}
