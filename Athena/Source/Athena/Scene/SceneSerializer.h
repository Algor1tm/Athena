#pragma once

#include "Athena/Core/Core.h"

#include "Scene.h"
#include "Entity.h"

namespace YAML
{
	class Emitter;
}

namespace Athena
{
	class ATHENA_API SceneSerializer
	{
	public:
		SceneSerializer(const Ref<Scene>& scene);

		void SerializeToFile(const FilePath& path);
		void SerializeRuntime(const FilePath& path);
		
		bool DeserializeFromFile(const FilePath& path);
		bool DeserializeRuntime(const FilePath& path);

	private:
		void SerializeEntity(YAML::Emitter& out, Entity entity);

	private:
		Ref<Scene> m_Scene;
	};
}
