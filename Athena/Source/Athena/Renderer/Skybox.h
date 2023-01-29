#pragma once

#include "Athena/Core/Core.h"


namespace Athena
{
	class ATHENA_API Cubemap;

	class ATHENA_API Skybox
	{
	public:
		static Ref<Skybox> Create(const Filepath& hdrMap);

		void Bind();
		const Filepath& GetFilepath() { return m_Path; };

	private:
		Ref<Cubemap> m_SkyboxCubemap;
		Ref<Cubemap> m_IrradianceMap;
		Ref<Cubemap> m_PrefilterMap;

		Filepath m_Path;
	};
}
