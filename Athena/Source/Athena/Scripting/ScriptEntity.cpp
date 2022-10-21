#include "ScriptEntity.h"
#include "ScriptEngine.h"

#include "Athena/Scene/Scene.h"

namespace Athena
{
	Vector3& ScriptEntity::GetTranslation()
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		
		Entity entity = scene->GetEntityByUUID(UUID(m_ID));
		return entity.GetComponent<TransformComponent>().Translation;
	}
}
