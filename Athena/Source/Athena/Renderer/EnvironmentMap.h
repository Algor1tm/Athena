#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Texture.h"


namespace Athena
{
	class ATHENA_API EnvironmentMap : public RefCounted
	{
	public:
		static Ref<EnvironmentMap> Create(const FilePath& hdrMap, uint32 resolution = 1024);

		const FilePath& GetFilePath() { return m_FilePath; };

		Ref<TextureCube> GetPrefilteredMap() { return m_PrefilteredMap; }
		Ref<TextureCube> GetIrradianceMap() { return m_IrradianceMap; }

		uint32 GetResolution() const { return m_Resolution; }
		void SetResolution(uint32 resolution);

	private:
		void Load();

	private:
		Ref<Texture2D> m_Panorama;
		Ref<TextureCube> m_PrefilteredMap;
		Ref<TextureCube> m_IrradianceMap;

		uint32 m_Resolution = 1024;
		uint32 m_IrradianceResolution = 64;

		FilePath m_FilePath;
	};
}
