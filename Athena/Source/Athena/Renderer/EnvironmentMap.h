#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Texture.h"


namespace Athena
{
	class ATHENA_API EnvironmentMap : public RefCounted
	{
	public:
		friend class ATHENA_API SceneRenderer;

	public:
		static Ref<EnvironmentMap> Create(const FilePath& hdrMap, uint32 resolution = 1024);

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
