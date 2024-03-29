#pragma once

#include "Athena/Core/Core.h"


namespace Athena
{
	class ATHENA_API TextureCube;

	class ATHENA_API EnvironmentMap
	{
	public:
		friend class ATHENA_API SceneRenderer;

	public:
		static Ref<EnvironmentMap> Create(const FilePath& hdrMap);

		void Bind();
		const FilePath& GetFilePath() { return m_FilePath; };

		uint32 GetResolution() const { return m_Resolution; }
		void SetResolution(uint32 resolution);

	private:
		void Load();

	private:
		Ref<TextureCube> m_PrefilteredMap;
		Ref<TextureCube> m_IrradianceMap;

		uint32 m_Resolution = 1024;
		uint32 m_IrradianceResolution = 64;

		FilePath m_FilePath;
	};
}
