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
		Ref<Cubemap> m_SkyboxMap;
		Ref<Cubemap> m_IrradianceMap;

		Filepath m_Path;
	};
}
