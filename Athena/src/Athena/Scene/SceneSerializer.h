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

		void SerializeToFile(const String& filepath);
		void SerializeRuntime(const String& filepath);
		
		bool DeserializeFromFile(const String& filepath);
		bool DeserializeRuntime(const String& filepath);

	private:
		void SerializeEntity(YAML::Emitter& out, Entity entity);

	private:
		Ref<Scene> m_Scene;
	};
}
