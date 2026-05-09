#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Scene/Scene.h"
#include "Athena/Scene/Entity.h"


namespace YAML
{
	class Emitter;
	class Node;
}

namespace Athena
{
	class ATHENA_API SceneSerializer
	{
	public:
		SceneSerializer(WeakRef<Scene> scene);

		bool SerializeToFile(const FilePath& path);
		bool DeserializeFromFile(const FilePath& path);

	private:
		void SerializeEntity(YAML::Emitter& out, Entity entity);

	private:
		WeakRef<Scene> m_Scene;
	};
}
